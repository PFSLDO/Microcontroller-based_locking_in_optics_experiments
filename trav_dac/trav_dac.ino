#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_MCP4725.h>

LiquidCrystal lcd(12, 14, 27, 26, 25, 33);
Adafruit_MCP4725 dac;

/////////////////////////////////////// CONSTANTES  E  DEFINIÇÕES

// já o ADC (leitura do fotodetector) tem resolução de 12 bits, de 0 à 4095

const adc1_channel_t adcChannel = ADC1_CHANNEL_0; // Canal 0 do ADC (GPIO36)

int resolution = 232;               // Inicializa a amplitude do sinal triangular como sendo ~ 3V, armazena o nível de amplitude
int resolutionmax = 255;           // Resolução de 8 bits
int delayUs = 20;                   // Delay em microssegundos entre as amostras
int numReadings = 10;          // Número de leituras para calcular a média
const float referenceVoltage = 3.3; // Tensão de referência (em volts)
unsigned long cycleStartTime = 0;   // Armazena o tempo de início do ciclo
unsigned long cycleEndTime = 0;     // Armazena o tempo de fim do ciclo
int frequency = 10;               // Inicializa a frequência do sinal em 10Hz, armazena a frequência
float amp_step = referenceVoltage/resolutionmax; // Dado necessário para o cálculo da mostra da amplitude
int step = 1;            // Passo para a rapidez do travamento
int averageAdcValue = 0; // Armazena o valor lido no ADC
int lastAdcValue = 0;    // Armazena o último valor lido no ADC
float amp = amp_step*resolution; //Calcula a amplitude
int direction = 1; // Variável para direcionar a aproximação do pico
float  value = 0;    // Variável de saída para o DAC
int delayInterrupt = 250;
double waiting_time = 1000000/(frequency*resolution*2); //Calcula o periodo esperado dado a frequência escolhida em microsegundos

const int buttonPin = 15;    // GPIO8 para alternar modos
const int increasePin = 4;   // GPIO4 para aumentar valores
const int decreasePin = 17;  // GPIO17 para diminuir valores
const int modeSwitchPin = 2; // GPIO2 para alternar entre varredura e travamento

bool optionButton = false;
bool increaseButton = false;
bool decreaseButton = false;
bool modeButton = false;

enum ModeSweep { AMPLITUDE, FREQUENCY }; // Vetor opções de configuração no modo de varredura
ModeSweep currentModeSweep = AMPLITUDE;       // e inicializa na amplitude
enum ModeLock { STEP, PEAK, AMOSTRAS };            // Vetor opções de configuração no modo de travamento
ModeLock currentModeLock = STEP;             // e inicializa no step
enum SystemMode { SWEEP, LOCK };         // Vetor dos modos de operação do sistema
SystemMode currentSystemMode = SWEEP;    // e inicializa na varredura

int targetValue = 0; // Armazena o valor do  DAC que representa o inicio do pico de interesse
const int peakThreshold = 40; // Limiar para detectar picos = 50mV 
const int resetThreshold = 20; // Limiar para redefinir a detecção de picos = 25mV
bool detectingPeak = false;     // Flag para indicação de detecção de picos
std::vector<unsigned int> peaks_place;  // Vetor para armazenar o valor de pzt respectivos para os picos
int currentPeakIndex = 0;       // Índice do pico atual

/////////////////////////////////////////////////// Função de interrupção dO BOTÃO DAS OPÇÕES DE CONFIGURAÇÃO
void IRAM_ATTR handleButtonPin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    optionButton = true;
    if (currentSystemMode == SWEEP) { 
      currentModeSweep = static_cast<ModeSweep>((currentModeSweep + 1) % 2);  // Alterna entre AMPLITUDE, FREQUENCY
    }
    else if (currentSystemMode == LOCK) { 
      currentModeLock = static_cast<ModeLock>((currentModeLock + 1) % 3);  // Alterna entre PEAK, STEP, AMOSTRAS
    }
  }
  lastInterruptTime = interruptTime;
}

/////////////////////////////////////////////////// Função de interrupção do BOTÃO DE INCREMENTO+
void IRAM_ATTR handleIncreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    increaseButton = true;
    if (currentSystemMode == SWEEP) { 
      switch (currentModeSweep) {
      case AMPLITUDE:
        resolution += 4; // Salto de aprox 50mV
        if (resolution > 255) resolution = 255;
        amp = amp_step*resolution;
        break;
      case FREQUENCY:
        frequency += 5;
        if (frequency > 50) frequency = 50;
        break;
      }
    waiting_time = 1000000/(frequency*resolution*2);
    }
    else if (currentSystemMode == LOCK) {
      switch (currentModeLock) {
        case PEAK:
          if (!peaks_place.empty()) {
            currentPeakIndex = (currentPeakIndex + 1) % peaks_place.size(); 
            targetValue = peaks_place[currentPeakIndex];
            value = targetValue;
          }
          break;
        case STEP:
          step++;
          break;
        case AMOSTRAS:
          numReadings++;
          break;
      }
    }
  }
  lastInterruptTime = interruptTime;
}


/////////////////////////////////////////////////// Função de interrupção do BOTÃO DE DECREMENTO-
void IRAM_ATTR handleDecreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    decreaseButton = true;
    if (currentSystemMode == SWEEP) { 
      switch (currentModeSweep) {
      case AMPLITUDE:
        resolution -= 4; // Salto de aprox 50mV
        if (resolution < 12) resolution = 12; // Limite  mínimo de amplitude de 150mV
        amp = amp_step*resolution;
        break;
      case FREQUENCY:
        frequency -= 5;
        if (frequency < 5) frequency = 5;
        break;
      }
    waiting_time = 1000000/(frequency*resolution*2);
    }
    else if (currentSystemMode == LOCK) { 
      switch (currentModeLock) {
      case PEAK:
        if (!peaks_place.empty()) {
          currentPeakIndex = (currentPeakIndex - 1 + peaks_place.size()) % peaks_place.size();
          targetValue = peaks_place[currentPeakIndex];
          value = targetValue;
        }
        break;
      case STEP:
        step--;
        if (step < 1) step = 1;
        break;
      case AMOSTRAS:
        numReadings--;
        if (numReadings < 1) numReadings = 1;
        break;
      }
    }
  }
  lastInterruptTime = interruptTime;
}


/////////////////////////////////////////////////// Função de interrupção do BOTÃO DO MODO DO SISTEMA
void IRAM_ATTR handleModeSwitchPin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    modeButton = true;
    currentSystemMode = static_cast<SystemMode>((currentSystemMode + 1) % 2);  // Alterna entre SWEEP e LOCK
    switch (currentSystemMode) {
      case SWEEP:
        value = 0;
        direction = 1;
        break;
      case LOCK:
        value = targetValue; // Inicia o sistema no início do pico escolhido
        lastAdcValue = averageAdcValue - 1; // 
        direction = 1;
        break;
    }
  }
  lastInterruptTime = interruptTime;
}

///////////////////////////////////////////////////// Configurações iniciais
void setup() {
  Serial.begin(115200);
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print("Travamento");
  lcd.setCursor(0, 1);
  lcd.print("Cavidade Triang");
  delay(2000);
  lcd.clear();

  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");

  // Configura o DAC
  Wire.begin();
  dac.begin(0x60);
  //Wire.begin(SDA_PIN, SCL_PIN);  // Ex: Wire.begin(21, 22);
  //dac.begin(0x60, &Wire);

  // Configura o ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_0);  // Sem atenuação

  // Inicializa o valor do DAC
  uint16_t valor12bit = map(0, 0, 255, 0, 4095);
  dac.setVoltage(valor12bit, false);

  // Configura os botões com interrupções
  pinMode(increasePin, INPUT_PULLUP);
  pinMode(decreasePin, INPUT_PULLUP);
  pinMode(modeSwitchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPin, FALLING);
  attachInterrupt(digitalPinToInterrupt(increasePin), handleIncreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreasePin), handleDecreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(modeSwitchPin), handleModeSwitchPin, FALLING);
}


//////////////////////////////////////////////// Função que ficará sendo eternamente executada
void loop() {
  if (currentSystemMode == SWEEP) {
    cycleStartTime = micros(); //Atualiza o valor de início da próxima iteração
    value += direction;
    if (value >= resolution) {
      value = resolution;
      direction = -1;
    } else if (value <= 0) {
      value = 0;
      direction = 1;
    } 
    int averageAdcValue = adc1_get_raw(adcChannel);
    // Detecção de picos (apenas quando a varredura está em direção positiva)
    if (direction == 1) {
      if (averageAdcValue > peakThreshold) {
        if(detectingPeak){
          peaks_place.pop_back();}
        peaks_place.push_back(value);
        detectingPeak = true; } }
    if (detectingPeak && averageAdcValue < resetThreshold) {
      detectingPeak = false; }

    //Atualizações do display após interrupções:
    if(increaseButton == true) {
      increaseButton = false;
      switch (currentModeSweep) {
      case AMPLITUDE:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      case FREQUENCY:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      }
    }
    if(decreaseButton == true) {
      decreaseButton = false;
      switch (currentModeSweep) {
      case AMPLITUDE:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      case FREQUENCY:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      }
    }
    if(optionButton == true) {
      optionButton = false;
      switch (currentModeSweep) {
      case AMPLITUDE:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      case FREQUENCY:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        break;
      }
    }
    if(modeButton == true) {
      modeButton = false;
      switch (currentSystemMode) {
      case SWEEP:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        if (currentModeSweep == 0) {
  lcd.setCursor(5, 0);
  lcd.print(">");
        } else if (currentModeSweep == 1) {
  lcd.setCursor(5, 1);
  lcd.print(">");
        }
      break;
      case LOCK:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lock   Step:");
  lcd.print(step);
  lcd.setCursor(1, 1);
  lcd.print("Pico:");
  lcd.print(currentPeakIndex+1);
  lcd.setCursor(7, 1);
  lcd.print("NRed:");
  lcd.print(numReadings);
        switch (currentModeLock) {
          case STEP:
  lcd.setCursor(6, 0);
  lcd.print(">");
          break;
          case PEAK:
  lcd.setCursor(1, 1);
  lcd.print(">");
          break;
          case AMOSTRAS:
  lcd.setCursor(6, 1);
  lcd.print(">");
          break;
        }
      break;
      } 
    }
    cycleEndTime = micros(); // Verifica o tempo que demorou nessa iteração
    while(cycleEndTime - cycleStartTime < waiting_time) { //Verifica se esse período de nivel ja ocorreu, se não ele continua preso no looping até dar o tempo
      cycleEndTime = micros();
    }

    uint16_t valor12bit = map(value, 0, 255, 0, 4095);
    dac.setVoltage(valor12bit, false);

  //////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  } else if (currentSystemMode == LOCK) {
    //Calcula o novo valor a ser passado para o PZT
    value += direction*step;
    uint16_t valor12bit = map(value, 0, 255, 0, 4095);
    dac.setVoltage(valor12bit, false);

    //Verifica a resposta do sistema, fazendo uma média na leitura para filtrar o ruído
    long adcSum = 0;
    for (int i = 0; i < numReadings; i++) {
      adcSum += adc1_get_raw(adcChannel);
      delayMicroseconds(delayUs);  // Pequeno delay entre leituras para evitar leituras muito rápidas
    }
    int averageAdcValue = adcSum / numReadings;

    // Executa a comparação e ajuste de direção caso necessário
    if (averageAdcValue > lastAdcValue) {
      direction = direction;
    } else if ( averageAdcValue < lastAdcValue) {
      direction = -direction;
    }

    lastAdcValue = averageAdcValue; // Atualiza a variável de última leitura do ADC 

    //Atualizações do display após interrupções:
    if(increaseButton == true) {
      increaseButton = false;
      switch (currentModeLock) {
      case STEP:
  lcd.setCursor(6, 0);
  lcd.print(">");
        break;
      case PEAK:
  lcd.setCursor(1, 1);
  lcd.print(">");
        break;
      case AMOSTRAS:
  lcd.setCursor(6, 1);
  lcd.print(">");
        break;
      }
    }
    if(decreaseButton == true) {
      decreaseButton = false;
      switch (currentModeLock) {
      case STEP:
  lcd.setCursor(6, 0);
  lcd.print(">");
        break;
      case PEAK:
  lcd.setCursor(1, 1);
  lcd.print(">");
        break;
      case AMOSTRAS:
  lcd.setCursor(6, 1);
  lcd.print(">");
        break;
      }
    }
    if(optionButton == true) {
      optionButton = false;
      switch (currentModeLock) {
      case STEP:
  lcd.setCursor(6, 0);
  lcd.print(">");
        break;
      case PEAK:
  lcd.setCursor(1, 1);
  lcd.print(">");
        break;
      case AMOSTRAS:
  lcd.setCursor(6, 1);
  lcd.print(">");
        break;
      }
    }
    if(modeButton == true) {
      modeButton = false;
      switch (currentSystemMode) {
      case SWEEP:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Sweep Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(6, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");
        if (currentModeSweep == 0) {
  lcd.setCursor(5, 0);
  lcd.print(">");
        } else if (currentModeSweep == 1) {
  lcd.setCursor(5, 1);
  lcd.print(">");
        }
      break;
      case LOCK:
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Lock   Step:");
  lcd.print(step);
  lcd.setCursor(1, 1);
  lcd.print("Pico:");
  lcd.print(currentPeakIndex+1);
  lcd.setCursor(7, 1);
  lcd.print("NRed:");
  lcd.print(numReadings);
        switch (currentModeLock) {
          case STEP:
  lcd.setCursor(6, 0);
  lcd.print(">");
          break;
          case PEAK:
  lcd.setCursor(1, 1);
  lcd.print(">");
          break;
          case AMOSTRAS:
  lcd.setCursor(6, 1);
  lcd.print(">");
          break;
        }
      break;
      } 
      
    }
  }
}