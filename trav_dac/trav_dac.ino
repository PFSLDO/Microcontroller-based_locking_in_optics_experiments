#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <LiquidCrystal.h>
#include <Adafruit_MCP4725.h>

LiquidCrystal lcd(12, 14, 27, 26, 25, 33);
Adafruit_MCP4725 dac;

const adc1_channel_t adcChannel = ADC1_CHANNEL_0; // Canal 0 do ADC (GPIO36)
const int dacBits = 12;
const int dacResolution = 4095;
const int resolutionmax = (1 << dacBits) - 1;
int resolution = int(0.91 * resolutionmax);              // Inicializa a amplitude do sinal triangular como sendo ~ 3V, armazena o nível de amplitude
const int dacSteps = 200;
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
float value = 0;    // Variável de saída para o DAC
int delayInterrupt = 250;
double waiting_time = 0; //Calcula o periodo esperado dado a frequência escolhida em microsegundos

const int buttonPin = 15;    // GPIO8 para alternar modos
const int increasePin = 4;   // GPIO4 para aumentar valores
const int decreasePin = 17;  // GPIO17 para diminuir valores
const int modeSwitchPin = 5; // GPIO2 para alternar entre varredura e travamento

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
const float peakThreshold_mV = 50.0;
const float resetThreshold_mV = 25.0;
const int peakThreshold = int((peakThreshold_mV / 1000.0) / referenceVoltage * resolutionmax);
const int resetThreshold = int((resetThreshold_mV / 1000.0) / referenceVoltage * resolutionmax);
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
        case AMPLITUDE: {
          int resolutionStep = int((0.050 / referenceVoltage) * resolutionmax);
          resolution += resolutionStep;

          if (resolution > resolutionmax) resolution = resolutionmax;
          amp = amp_step*resolution;
          break;
        }
        case FREQUENCY: {
         frequency += 5;

          if (frequency > 50) frequency = 50;
         break;
        }
      }

    waiting_time = (1000000.0 / frequency) / (2.0 * resolution);
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
        case AMPLITUDE: {
          // Salto de aproximadamente 50 mV convertido para unidades do DAC
          int resolutionStep = int((0.050 / referenceVoltage) * resolutionmax);
          int resolutionMin = int((0.150 / referenceVoltage) * resolutionmax); // 150 mV

          resolution -= resolutionStep;
          if (resolution < resolutionMin) resolution = resolutionMin;

          amp = amp_step * resolution;
          break;
        }

        case FREQUENCY: {
          frequency -= 5;

          if (frequency < 5) frequency = 5;
          break;
        }
      }

    waiting_time = (1000000.0 / frequency) / (2.0 * resolution);
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
        value = targetValue;
        lastAdcValue = averageAdcValue - 1;
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
  lcd.print("Sweep >Amp:");
  lcd.print(amp);
  lcd.print("V");
  lcd.setCursor(7, 1);
  lcd.print("Freq:");
  lcd.print(frequency);
  lcd.print("Hz");

  Wire.begin();
  dac.begin(0x60);

  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_0);  // Sem atenuação
  dac.setVoltage(value, false);

  pinMode(increasePin, INPUT_PULLUP);
  pinMode(decreasePin, INPUT_PULLUP);
  pinMode(modeSwitchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPin, FALLING);
  attachInterrupt(digitalPinToInterrupt(increasePin), handleIncreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreasePin), handleDecreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(modeSwitchPin), handleModeSwitchPin, FALLING);

  waiting_time = 1000000.0 / (frequency * dacSteps * 2); // em microssegundos
  Serial.print("Waiting time per step (us): ");
  Serial.println(waiting_time);
}

void loop() {
  static unsigned long lastUpdate = 0;

  if (currentSystemMode == SWEEP) {
    unsigned long now = micros();
    if (now - lastUpdate >= waiting_time) {
      lastUpdate = now;

      // Atualiza valor do DAC
      value += direction;

      // Inverte direção nos limites
      if (value >= dacSteps) {
        value = dacSteps;
        direction = -1;
      }
      if (value <= 0) {
        value = 0;
        direction = 1;
      }

      // Mapeia valor para resolução do DAC e envia
      int dacValue = int(value * (dacResolution / float(dacSteps)));
      dac.setVoltage(dacValue, false);
    }
    
    //Atualizações do display após interrupções:
    if (increaseButton == true) {
      increaseButton = false;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sweep  Amp:");
      lcd.print(amp);
      lcd.print("V");
      lcd.setCursor(7, 1);
      lcd.print("Freq:");
      lcd.print(frequency);
      lcd.print("Hz");
      lcd.setCursor(6, 0);
      lcd.print(" ");
      lcd.setCursor(6, 1);
      lcd.print(" ");
      
      if (currentModeSweep == 0) {
        lcd.setCursor(6, 0);
        lcd.print(">");
      } else if (currentModeSweep == 1) {
        lcd.setCursor(6, 1);
        lcd.print(">");
      }
    }
    if (decreaseButton == true) {
      decreaseButton = false;
  
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sweep  Amp:");
      lcd.print(amp);
      lcd.print("V");
      lcd.setCursor(7, 1);
      lcd.print("Freq:");
      lcd.print(frequency);
      lcd.print("Hz");

      lcd.setCursor(6, 0);
      lcd.print(" ");
      lcd.setCursor(6, 1);
      lcd.print(" ");
      
      if (currentModeSweep == 0) {
        lcd.setCursor(6, 0);
        lcd.print(">");
      } else if (currentModeSweep == 1) {
        lcd.setCursor(6, 1);
        lcd.print(">");
      }
    }
    if(optionButton == true) {
      optionButton = false;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sweep  Amp:");
      lcd.print(amp);
      lcd.print("V");
      lcd.setCursor(7, 1);
      lcd.print("Freq:");
      lcd.print(frequency);
      lcd.print("Hz");

      lcd.setCursor(6, 0);
      lcd.print(" ");
      lcd.setCursor(6, 1);
      lcd.print(" ");
      
      if (currentModeSweep == 0) {
        lcd.setCursor(6, 0);
        lcd.print(">");
      } else if (currentModeSweep == 1) {
        lcd.setCursor(6, 1);
        lcd.print(">");
      }
    }

    if(modeButton == true) {
      modeButton = false;
      
      switch (currentSystemMode) {
        case SWEEP:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Sweep  Amp:");
          lcd.print(amp);
          lcd.print("V");
          lcd.setCursor(7, 1);
          lcd.print("Freq:");
          lcd.print(frequency);
          lcd.print("Hz");
          lcd.setCursor(6, 0);
          lcd.print(" ");
          lcd.setCursor(6, 1);
          lcd.print(" ");
        
          if (currentModeSweep == 0) {
            lcd.setCursor(6, 0);
            lcd.print(">");
          } else if (currentModeSweep == 1) {
            lcd.setCursor(6, 1);
            lcd.print(">");
          }
      
        break;
        case LOCK:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Lock     Step:");
          lcd.print(step);
          lcd.setCursor(1, 1);
          lcd.print("Pico:");
          lcd.print(currentPeakIndex+1);
          lcd.setCursor(9, 1);
          lcd.print("NRead:");
          lcd.print(numReadings);
          lcd.setCursor(8, 0);
          lcd.print(" ");
          lcd.setCursor(0, 1);
          lcd.print(" ");
          lcd.setCursor(8, 1);
          lcd.print(" ");
          
          switch (currentModeLock) {
            case STEP:
              lcd.setCursor(8, 0);
              lcd.print(">");
            break;
            case PEAK:
              lcd.setCursor(0, 1);
              lcd.print(">");
            break;
            case AMOSTRAS:
              lcd.setCursor(8, 1);
              lcd.print(">");
            break;
          }
          
        break;
      } 
    }
    //cycleEndTime = micros(); // Verifica o tempo que demorou nessa iteração
    //
    //while (cycleEndTime - cycleStartTime < waiting_time) { //Verifica se esse período de nivel ja ocorreu, se não ele continua preso no looping até dar o tempo
    //  cycleEndTime = micros();
    //}

    //dac.setVoltage(value, false);

  } else if (currentSystemMode == LOCK) {
    float dacSampleRate = 1000000.0 / waiting_time; // em Hz
    int pointsPerHalfCycle = (int)(dacSampleRate / (2 * frequency));
    int stepSize = resolution / pointsPerHalfCycle;
    value += direction * stepSize;
    //value += direction*step;
    value = constrain(value, 0, resolutionmax);
    dac.setVoltage(value, false);

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
    if (increaseButton == true) {
      increaseButton = false;

      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lock     Step:");
      lcd.print(step);
      lcd.setCursor(1, 1);
      lcd.print("Pico:");
      lcd.print(currentPeakIndex+1);
      lcd.setCursor(9, 1);
      lcd.print("NRead:");
      lcd.print(numReadings);
      lcd.setCursor(8, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(8, 1);
      lcd.print(" ");
      
      switch (currentModeLock) {
        case STEP:
          lcd.setCursor(8, 0);
          lcd.print(">");
        break;
        case PEAK:
          lcd.setCursor(0, 1);
          lcd.print(">");
        break;
        case AMOSTRAS:
          lcd.setCursor(8, 1);
          lcd.print(">");
        break;
      }
    }
    if (decreaseButton == true) {
      decreaseButton = false;
      
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Lock     Step:");
      lcd.print(step);
      lcd.setCursor(1, 1);
      lcd.print("Pico:");
      lcd.print(currentPeakIndex+1);
      lcd.setCursor(9, 1);
      lcd.print("NRead:");
      lcd.print(numReadings);

      lcd.setCursor(8, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(8, 1);
      lcd.print(" ");
      
      switch (currentModeLock) {
        case STEP:
          lcd.setCursor(8, 0);
          lcd.print(">");
        break;
        case PEAK:
          lcd.setCursor(0, 1);
          lcd.print(">");
        break;
        case AMOSTRAS:
          lcd.setCursor(8, 1);
          lcd.print(">");
        break;
      }
    }

    if(optionButton == true) {
      optionButton = false;

      lcd.setCursor(8, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(8, 1);
      lcd.print(" ");
      
      switch (currentModeLock) {
        case STEP:
          lcd.setCursor(8, 0);
          lcd.print(">");
        break;
        case PEAK:
          lcd.setCursor(0, 1);
          lcd.print(">");
        break;
        case AMOSTRAS:
          lcd.setCursor(8, 1);
          lcd.print(">");
        break;
      }
    }

    if (modeButton == true) {
      modeButton = false;
      
      switch (currentSystemMode) {
        case SWEEP:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Sweep  Amp:");
          lcd.print(amp);
          lcd.print("V");
          lcd.setCursor(7, 1);
          lcd.print("Freq:");
          lcd.print(frequency);
          lcd.print("Hz");
          lcd.setCursor(6, 0);
          lcd.print(" ");
          lcd.setCursor(6, 1);
          lcd.print(" ");
        
          if (currentModeSweep == 0) {
            lcd.setCursor(6, 0);
            lcd.print(">");
          } else if (currentModeSweep == 1) {
            lcd.setCursor(6, 1);
            lcd.print(">");
          }
      
        break;
        case LOCK:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Lock     Step:");
          lcd.print(step);
          lcd.setCursor(1, 1);
          lcd.print("Pico:");
          lcd.print(currentPeakIndex+1);
          lcd.setCursor(9, 1);
          lcd.print("NRead:");
          lcd.print(numReadings);
          lcd.setCursor(8, 0);
          lcd.print(" ");
          lcd.setCursor(0, 1);
          lcd.print(" ");
          lcd.setCursor(8, 1);
          lcd.print(" ");

          switch (currentModeLock) {
            case STEP:
              lcd.setCursor(8, 0);
              lcd.print(">");
            break;
            case PEAK:
              lcd.setCursor(0, 1);
              lcd.print(">");
            break;
            case AMOSTRAS:
              lcd.setCursor(8, 1);
              lcd.print(">");
            break;
          }
      
        break;
      } 
    }
  }
}