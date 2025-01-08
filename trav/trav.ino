#include <adafruit_Ggfx.h>
#include <adafruit_ssd1306.h>
#include <driver/adc.h>
#include <driver/dac.h>
#include <esp_adc_cal.h>
#include <spo.h>
#include <vector>
#include <wire.h>

#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

//--------------------------------------------------------------------------------------------
// o DAC (saida para o PZT) tem resolucao de 8 bits (0 a 255)
// ja o ADC (leitura do fotodetector) tem resolucao de 12 bits (0 a 4095)

const dac_channel_t dacChannel = DAC_CHANNEL_1;   // Canal 1 do DAC (GPIO25)
const adc1_channel_t adcChannel = ADC1_CHANNEL_0; // Canal 0 do ADC (GPIO36)

const int buttonPin = 15;      // GPIO8 para alternar modos
const int increasePin = 4;     // GPIO4 para aumentar valores
const int decreasePin = 17;    // GPIO17 para diminuir valores
const int modeSwitchPin = 2;   // GPIO2 para alternar entre varredura e travamento
const int peakThreshold = 40;  // Limiar para detectar picos = 50mV 
const int resetThreshold = 20; // Limiar para redefinir a deteccao de picos = 25mV

bool optionButton = false;
bool increaseButton = false;
bool decreaseButton = false;
bool modeButton = false;
bool detectingPeak = false;

float value = 0;                                 // Variavel de saida para o DAC
const float referenceVoltage = 3.3;              // Tensao de referencia (volts)
unsigned long cycleStartTime = 0;                // Armazena o tempo de inicio do ciclo
unsigned long cycleEndTime = 0;                  // Armazena o tempo de fim do ciclo
int triangularSignalResolution = 232;            // Inicializa a amplitude do sinal triangular como sendo ~ 3V, armazena o nivel de amplitude
int resolutionmax = 255;                         // Resolucao de 8 bits
int delayUs = 20;                                // Delay em microssegundos entre as amostras
int numReadings = 10;                            // Numero de leituras para calcular a media
int frequency = 10;                              // Inicializa a frequencia do sinal em 10Hz, armazena a frequencia
int step = 1;                                    // Passo para a rapidez do travamento
int averageAdcValue = 0;                         // Armazena o valor lido no ADC
int lastAdcValue = 0;                            // Armazena o ultimo valor lido no ADC
int direction = 1;                               // Variavel para direcionar a aproximacao do pico
int targetValue = 0;                             // Armazena o valor do  DAC que representa o inicio do pico de interesse
int currentPeakIndex = 0;                        // Indice do pico atual
int delayInterrupt = 250;

std::vector<unsigned int> peaks_place;           // Vetor para armazenar o valor de pzt respectivos para os picos

float amp_step = referenceVoltage / resolutionmax;                            // Dado necessario para o calculo da mostra da amplitude
float amp = amp_step * triangularSignalResolution;                            // Calcula a amplitude
double waiting_time = 1000000 / (frequency * triangularSignalResolution * 2); // Calcula o periodo esperado dado a frequencia escolhida em microsegundos

enum ModeSweep { AMPLITUDE, FREQUENCY };
enum ModeLock { STEP, PEAK, AMOSTRAS };
enum SystemMode { SWEEP, LOCK };
ModeSweep currentModeSweep = AMPLITUDE;
ModeLock currentModeLock = STEP;
SystemMode currentSystemMode = SWEEP;

//--------------------------------------------------------------------------------------------
// Funcao de interrupcao do botao "opcoes de configuracao"
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

// Funcao de interrupcao do botao "incremento+"
void IRAM_ATTR handleIncreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    increaseButton = true;
    if (currentSystemMode == SWEEP) { 
      switch (currentModeSweep) {
      case AMPLITUDE:
        triangularSignalResolution += 4; // Salto de aprox 50mV
        if (triangularSignalResolution > 255) triangularSignalResolution = 255;
        amp = amp_step*triangularSignalResolution;
        break;
      case FREQUENCY:
        frequency += 5;
        if (frequency > 50) frequency = 50;
        break;
      }
    waiting_time = 1000000/(frequency*triangularSignalResolution*2);
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

// Funcao de interrupcao do botao "incremento-"
void IRAM_ATTR handleDecreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    decreaseButton = true;
    if (currentSystemMode == SWEEP) { 
      switch (currentModeSweep) {
      case AMPLITUDE:
        triangularSignalResolution -= 4; // Salto de aprox 50mV
        if (triangularSignalResolution < 12) triangularSignalResolution = 12; // Limite  mínimo de amplitude de 150mV
        amp = amp_step*triangularSignalResolution;
        break;
      case FREQUENCY:
        frequency -= 5;
        if (frequency < 5) frequency = 5;
        break;
      }
    waiting_time = 1000000/(frequency*triangularSignalResolution*2);
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

// Funcao de interrupcao do botao "modo do sistema"
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

//--------------------------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  // configura o display oled
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(15,8);
  display.print("Modo:Varredura");
  display.drawLine(15, 17, 15+14*6, 17, WHITE);
  display.setCursor(15,30);
  display.print("Amplitude:");
  display.print(amp);
  display.print("V");
  display.setCursor(15,50);
  display.print("Frequencia:");
  display.print(frequency);
  display.print("Hz");
  display.setCursor(0,30);
  display.write(62);
  display.display();

  // Configura o DAC
  dac_output_enable(dacChannel);

  // Configura o ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_0);  // Sem atenuação

  // Inicializa o valor do DAC
  dac_output_voltage(dacChannel, 0);

  // Configura os botoes com interrupções
  pinMode(increasePin, INPUT_PULLUP);
  pinMode(decreasePin, INPUT_PULLUP);
  pinMode(modeSwitchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPin, FALLING);
  attachInterrupt(digitalPinToInterrupt(increasePin), handleIncreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreasePin), handleDecreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(modeSwitchPin), handleModeSwitchPin, FALLING);
}

//--------------------------------------------------------------------------------------------

void loop() {
  if (currentSystemMode == SWEEP) {
    cycleStartTime = micros(); //Atualiza o valor de início da próxima iteração
    value += direction;

    if (value >= triangularSignalResolution) {
      value = triangularSignalResolution;
      direction = -1;
    }
    else if (value <= 0) {
      value = 0;
      direction = 1;
    }

    int averageAdcValue = adc1_get_raw(adcChannel);

    if (direction == 1) { 
      if (averageAdcValue > peakThreshold) {
        if (detectingPeak) { // Deteccao de picos (apenas quando a varredura estah em direcao positiva)
          peaks_place.pop_back();
        }

        peaks_place.push_back(value);
        detectingPeak = true;
      }
    }

    if (detectingPeak && averageAdcValue < resetThreshold) {
      detectingPeak = false;
    }

    //Atualizacoes do display apos interrupcoes
    if (increaseButton == true) {
      increaseButton = false;
      
      switch (currentModeSweep) {
        case AMPLITUDE:
          display.fillRect(15, 30, 100, 8, BLACK);
          display.setCursor(15,30);
          display.print("Amplitude:");
          display.print(amp);
          display.print("V");
        break;
        case FREQUENCY:
          display.fillRect(15, 50, 100, 8, BLACK);
          display.setCursor(15,50);
          display.print("Frequencia:");
          display.print(frequency);
          display.print("Hz");
        break;
      }

      display.display();
    }
    
    if (decreaseButton == true) {
      decreaseButton = false;
      
      switch (currentModeSweep) {
        case AMPLITUDE:
          display.fillRect(15, 30, 100, 8, BLACK);
          display.setCursor(15,30);
          display.print("Amplitude:");
          display.print(amp);
          display.print("V");
        break;
        case FREQUENCY:
          display.fillRect(15, 50, 100, 8, BLACK);
          display.setCursor(15,50);
          display.print("Frequencia:");
          display.print(frequency);
          display.print("Hz");
        break;
      }
      
      display.display();
    }
    if  (optionButton == true) {
      optionButton = false;
      
      switch (currentModeSweep) {
        case AMPLITUDE:
          display.setCursor(0,30);
          display.write(62);
          display.fillRect(0, 50, 6, 8, BLACK);
          display.display();
        break;
        case FREQUENCY:
          display.setCursor(0,50);
          display.write(62);
          display.fillRect(0, 30, 6, 8, BLACK);
        break;
      }
      
      display.display();
    }
    
    if (modeButton == true) {
      modeButton = false;
      
      switch (currentSystemMode) {
        case SWEEP:
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.clearDisplay();
          display.setCursor(15,8);
          display.print("Modo:Varredura");
          display.drawLine(15, 17, 15+14*6, 17, WHITE);
          display.setCursor(15,30);
          display.print("Amplitude:");
          display.print(amp);
          display.print("V:");
          display.setCursor(15,50);
          display.print("Frequencia:");
          display.print(frequency);
          display.print("Hz");
        
          if (currentModeSweep == 0) {
            display.setCursor(0,30);
            display.write(62);
          }
          else if (currentModeSweep == 1) {
            display.setCursor(0,50);
            display.write(62);
          }
        
          display.display();
        break;
        case LOCK:
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.clearDisplay();
          display.setCursor(15,8);
          display.print("Modo:Travamento");
          display.drawLine(15, 17, 15+15*6, 17, WHITE);
          display.setCursor(15,20);
          display.print("Step:");
          display.print(step);
          display.setCursor(15,32);
          display.print("Pico:");
          display.print(currentPeakIndex+1);
          display.setCursor(15,44);
          display.print("Amostras media:");
          display.print(numReadings);
        
        switch (currentModeLock) {
          case STEP:
            display.setCursor(0,20);
            display.write(62);
          break;
          case PEAK:
            display.setCursor(0,32);
            display.write(62);
          break;
          case AMOSTRAS:
            display.setCursor(0,44);
            display.write(62);
          break;
        }

        display.display();
        break;
      }
    }
    cycleEndTime = micros(); // Verifica o tempo que demorou nessa iteração
    
    while(cycleEndTime - cycleStartTime < waiting_time) { //Verifica se esse período de nivel ja ocorreu, se não ele continua preso no looping até dar o tempo
      cycleEndTime = micros();
    }

    dac_output_voltage(dacChannel, value); // atualiza o valor na saída
  }
  else if (currentSystemMode == LOCK) {
    //Calcula o novo valor a ser passado para o PZT
    value += direction*step;
    dac_output_voltage(dacChannel, value); // Atualiza o valor na saída

    //Verifica a resposta do sistema, fazendo uma média na leitura para filtrar o ruído
    long adcSum = 0;
    for (int i = 0; i < numReadings; i++) {
      adcSum += adc1_get_raw(adcChannel);
      delayMicroseconds(delayUs);  // Pequeno delay entre leituras para evitar bug de leituras muito rápidas
    }

    int averageAdcValue = adcSum / numReadings;

    // Executa a comparação e ajuste de direção caso necessário
    if (averageAdcValue > lastAdcValue) {
      direction = direction;
    }
    else if ( averageAdcValue < lastAdcValue) {
      direction = -direction;
    }

    lastAdcValue = averageAdcValue; // Atualiza a variável de última leitura do ADC 

    //Atualizacoes do display apos interrupcoes
    if (increaseButton == true) {
      increaseButton = false;
      switch (currentModeLock) {
        case STEP:
          display.fillRect(15, 20, 100, 8, BLACK);
          display.setCursor(15,20);
          display.print("Step:");
          display.print(step);
        break;
        case PEAK:
          display.fillRect(15, 32, 100, 8, BLACK);
          display.setCursor(15,32);
          display.print("Pico:");
          display.print(currentPeakIndex+1);
        break;
        case AMOSTRAS:
          display.fillRect(15, 44, 100, 8, BLACK);
          display.setCursor(15,44);
          display.print("Amostras media:");
          display.print(numReadings);
        break;
      }

      display.display();
    }
    if (decreaseButton == true) {
      decreaseButton = false;
      
      switch (currentModeLock) {
        case STEP:
          display.fillRect(15, 20, 100, 8, BLACK);
          display.setCursor(15,20);
          display.print("Step:");
          display.print(step);
        break;
        case PEAK:
          display.fillRect(15, 32, 100, 8, BLACK);
          display.setCursor(15,32);
          display.print("Pico:");
          display.print(currentPeakIndex+1);
        break;
        case AMOSTRAS:
          display.fillRect(15, 44, 100, 8, BLACK);
          display.setCursor(15,44);
          display.print("Amostras media:");
          display.print(numReadings);
        break;
      }

      display.display();
    }
    if (optionButton == true) {
      optionButton = false;
      
      switch (currentModeLock) {
        case STEP:
          display.setCursor(0,20);
          display.write(62);
          display.fillRect(0, 32, 6, 8, BLACK);
          display.fillRect(0, 44, 6, 8, BLACK);
          display.fillRect(0, 56, 6, 8, BLACK);
          display.display();
        break;
        case PEAK:
          display.setCursor(0,32);
          display.write(62);
          display.fillRect(0, 20, 6, 8, BLACK);
          display.fillRect(0, 44, 6, 8, BLACK);
          display.fillRect(0, 56, 6, 8, BLACK);
        break;
        case AMOSTRAS:
          display.setCursor(0,44);
          display.write(62);
          display.fillRect(0, 20, 6, 8, BLACK);
          display.fillRect(0, 32, 6, 8, BLACK);
          display.fillRect(0, 56, 6, 8, BLACK);
        break;
      }

      display.display();
    }

    if (modeButton == true) {
      modeButton = false;
      
      switch (currentSystemMode) {
        case SWEEP:
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.clearDisplay();
          display.setCursor(15,10);
          display.print("Modo:Varredura");
          display.drawLine(15, 19, 15+14*6, 19, WHITE);
          display.setCursor(15,30);
          display.print("Amplitude:");
          display.print(amp);
          display.print("V:");
          display.setCursor(15,50);
          display.print("Frequencia:");
          display.print(frequency);
          display.print("Hz");
        
          if (currentModeSweep == 0) {
            display.setCursor(0,30);
            display.write(62);
          }
          else if (currentModeSweep == 1) {
            display.setCursor(0,50);
            display.write(62);
          }
        
          display.display();
        break;
        case LOCK:
          display.setTextSize(1);
          display.setTextColor(WHITE);
          display.clearDisplay();
          display.setCursor(15,8);
          display.print("Modo:Travamento");
          display.drawLine(15, 17, 15+15*6, 17, WHITE);
          display.setCursor(15,20);
          display.print("Step:");
          display.print(step);
          display.setCursor(15,32);
          display.print("Pico:");
          display.print(currentPeakIndex+1);
          display.setCursor(15,44);
          display.print("Amostras media:");
          display.print(numReadings);
        
          switch (currentModeLock) {
            case STEP:
              display.setCursor(0,20);
              display.write(62);
            break;
            case PEAK:
              display.setCursor(0,32);
              display.write(62);
            break;
            case AMOSTRAS:
              display.setCursor(0,44);
              display.write(62);
            break;
          }
        
          display.display();
        break;
      } 
    }
  }
}