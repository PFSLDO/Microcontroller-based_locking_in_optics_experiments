#include <driver/dac.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <vector>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define LARGURA_OLED 128
#define ALTURA_OLED 64
 
#define RESET_OLED -1
 
Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

/////////////////////////////////////// CONSTANTES  E  DEFINIÇÕES

const dac_channel_t dacChannel = DAC_CHANNEL_1;   // Canal 1 do DAC (GPIO25)
const adc1_channel_t adcChannel = ADC1_CHANNEL_0; // Canal 0 do ADC (GPIO36)

int resolution = 993;               // Inicializa a amplitude do sinal triangular como sendo ~ 800mV, armazena o nível de amplitude
int resolutionmax = 4095;           // Resolução de 12 bits
int delayUs = 20;                   // Delay em microssegundos entre as amostras
const int numReadings = 3;          // Número de leituras para calcular a média
const float referenceVoltage = 3.3; // Tensão de referência (em volts)
unsigned long cycleStartTime = 0;   // Armazena o tempo de início do ciclo
unsigned long cycleEndTime = 0;     // Armazena o tempo de fim do ciclo
float frequency = 10;               // Inicializa a frequência do sinal em 10Hz, armazena a frequência
int amp_step = referenceVoltage/resolutionmax; // Dado necessário para o cálculo da mostra da amplitude
int step = 1;            // Passo para a rapidez do travamento
int averageAdcValue = 0; // Armazena o valor lido no ADC
int lastAdcValue = 0;    // Armazena o último valor lido no ADC
int amp = amp_step*resolution; //Calcula a amplitude
int direction = 1; // Variável para direcionar a aproximação do pico
int  value = 0;    // Variável de saída para o DAC

const int buttonPin = 15;    // GPIO8 para alternar modos
const int increasePin = 4;   // GPIO4 para aumentar valores
const int decreasePin = 17;  // GPIO17 para diminuir valores
const int modeSwitchPin = 2; // GPIO2 para alternar entre varredura e travamento

enum ModeSweep { AMPLITUDE, FREQUENCY }; // Vetor opções de configuração no modo de varredura
ModeSweep currentModeSweep = AMPLITUDE;       // e inicializa na amplitude
enum ModeLock { STEP, PEAK };            // Vetor opções de configuração no modo de travamento
ModeLock currentModeLock = STEP;             // e inicializa no step
enum SystemMode { SWEEP, LOCK };         // Vetor dos modos de operação do sistema
SystemMode currentSystemMode = SWEEP;    // e inicializa na varredura

int targetValue = 0; // Armazena o valor do  DAC que representa o inicio do pico de interesse
const int peakThreshold = 250;  // Limiar para detectar picos = 200mV
const int resetThreshold = 62; // Limiar para redefinir a detecção de picos = 50mV
bool detectingPeak = false;      // Flag para indicação de detecção de picos
std::vector<int> peaks;          // Vetor para armazenar os picos detectados
int currentPeakIndex = 0;        // Índice do pico atual


/////////////////////////////////////////////////// Função de interrupção dO BOTÃO DAS OPÇÕES DE CONFIGURAÇÃO
void IRAM_ATTR handleButtonPin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) {
    if (currentSystemMode == SWEEP) { 
      currentModeSweep = static_cast<ModeSweep>((currentModeSweep + 1) % 2);  // Alterna entre AMPLITUDE, FREQUENCY
      switch (currentModeSweep) {
      case AMPLITUDE:
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Varredura");
        display.setCursor(15,20);
        display.print("Amplitude:");
        display.setCursor(35,20);
        display.print(amp);
        display.setCursor(15,30);
        display.print("Frequência:");
        display.setCursor(35,30);
        display.print(frequency);
        display.setCursor(0,20);
        display.write(62);
        break;
      case FREQUENCY:
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Varredura");
        display.setCursor(15,20);
        display.print("Amplitude:");
        display.setCursor(35,20);
        display.print(amp);
        display.setCursor(15,30);
        display.print("Frequência:");
        display.setCursor(35,30);
        display.print(frequency);
        display.setCursor(0,30);
        display.write(62);
        break;
      }
    }
    else if (currentSystemMode == LOCK) { 
      currentModeLock = static_cast<ModeLock>((currentModeLock + 1) % 2);  // Alterna entre PEAK, STEP
      switch (currentModeLock) {
      case PEAK:
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Varredura");
        display.setCursor(15,20);
        display.print("Amplitude:");
        display.setCursor(35,20);
        display.print(amp);
        display.setCursor(15,30);
        display.print("Frequência:");
        display.setCursor(35,30);
        display.print(frequency);
        display.setCursor(0,30);
        display.write(62);
        break;
      case STEP:
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Varredura");
        display.setCursor(15,20);
        display.print("Amplitude:");
        display.setCursor(35,20);
        display.print(amp);
        display.setCursor(15,30);
        display.print("Frequência:");
        display.setCursor(35,30);
        display.print(frequency);
        display.setCursor(0,20);
        display.write(62);
        break;
      }
    }
  }
  lastInterruptTime = interruptTime;
}

/////////////////////////////////////////////////// Função de interrupção do BOTÃO DE INCREMENTO+
void IRAM_ATTR handleIncreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > 200) {
    if (currentSystemMode == SWEEP) { 
      currentModeSweep = static_cast<ModeSweep>((currentModeSweep + 1) % 2);  // Alterna entre AMPLITUDE, FREQUENCY
      switch (currentModeSweep) {
      case AMPLITUDE:
        resolution += 62;
        if (resolution > 4095) resolution = 4095;
        amp = amp_step*resolution;
        display.setCursor(35,20);
        display.print(amp);
        break;
      case FREQUENCY:
        frequency += 5;
        if (frequency > 50) frequency = 50;
        display.setCursor(35,30);
        display.print(frequency);
        break;
      }
    }
    else if (currentSystemMode == LOCK) {
      currentModeLock = static_cast<ModeLock>((currentModeLock + 1) % 2);  // Alterna entre PEAK, STEP
      switch (currentModeLock) {
        case PEAK:
        if (!peaks.empty()) {
          currentPeakIndex = (currentPeakIndex + 1) % peaks.size(); 
          targetValue = peaks[currentPeakIndex];
          display.setCursor(35,30);
          display.print(currentPeakIndex);
        }
        break;
        case STEP:
        step++;
        display.setCursor(35,20);
        display.print(step);
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
  if (interruptTime - lastInterruptTime > 200) {
    if (currentSystemMode == SWEEP) { 
      currentModeSweep = static_cast<ModeSweep>((currentModeSweep + 1) % 2);  // Alterna entre AMPLITUDE, FREQUENCY
      switch (currentModeSweep) {
      case AMPLITUDE:
        resolution -= 62;
        if (resolution < 186) resolution = 186; // Limite  mínimo de amplitude de 150mV
        display.setCursor(35,20);
        display.print(amp);
        break;
      case FREQUENCY:
        frequency -= 5;
        if (frequency < 5) frequency = 5;
        display.setCursor(35,30);
        display.print(frequency);
        break;
      }
    }
    else if (currentSystemMode == LOCK) { 
      currentModeLock = static_cast<ModeLock>((currentModeLock + 1) % 2);  // Alterna entre PEAK, STEP
      switch (currentModeLock) {
      case PEAK:
        if (!peaks.empty()) {
          currentPeakIndex = (currentPeakIndex - 1 + peaks.size()) % peaks.size();
          targetValue = peaks[currentPeakIndex];
          display.setCursor(35,30);
          display.print(currentPeakIndex);
        }
        break;
      case STEP:
        step--;
        if (step < 1) step = 1;
        display.setCursor(35,20);
        display.print(step);
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
  if (interruptTime - lastInterruptTime > 200) {
    currentSystemMode = static_cast<SystemMode>((currentSystemMode + 1) % 2);  // Alterna entre SWEEP e LOCK
    switch (currentSystemMode) {
      case SWEEP:
        value = 0;
        direction = 1;
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Varredura");
        display.setCursor(15,20);
        display.print("Amplitude:");
        display.setCursor(35,20);
        display.print(amp);
        display.setCursor(15,30);
        display.print("Frequência:");
        display.setCursor(35,30);
        display.print(frequency);
        if (currentModeSweep == 0) {
          display.setCursor(0,20);
          display.write(62);
        } else if (currentModeSweep == 1) {
          display.setCursor(0,30);
          display.write(62);
        }
        break;
      case LOCK:
        if (!peaks.empty()) {
          targetValue = peaks[0];  // Inicializa o modo LOCK no primeiro pico
        }
        value = targetValue; // Inicia o sistema no início do pico escolhido
        lastAdcValue = averageAdcValue - 1; // 
        direction = 1;
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.clearDisplay();
        display.setCursor(15,10);
        display.print("Modo: Travamento");
        display.setCursor(15,20);
        display.print("Step:");
        display.setCursor(35,20);
        display.print(step);
        display.setCursor(15,30);
        display.print("Pico:");
        display.setCursor(35,30);
        display.print(currentPeakIndex);
        if (currentModeSweep == 0) {
          display.setCursor(0,20);
          display.write(62);
        } else if (currentModeSweep == 1) {
          display.setCursor(0,30);
          display.write(62);
        }
        break;
    }
  }
  lastInterruptTime = interruptTime;
}

///////////////////////////////////////////////////// Configurações iniciais
void setup() {
  // configura o display oled
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();

  // Configura o DAC
  dac_output_enable(dacChannel);

  // Configura o ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_11);  // Atenuação de 11dB

  // Inicializa o valor do DAC
  dac_output_voltage(dacChannel, 0);

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
    // Gera o sinal triangular
    value += direction;
    if (value >= resolution) {
      value = resolution;
      direction = -1;
    } else if (value <= 0) {
      value = 0;
      direction = 1;
    }

    // Lê a entrada do ADC 
    averageAdcValue = adc1_get_raw(adcChannel);    

    // Detecção de picos (apenas quando a varredura está em direção positiva)
    if (direction == 1) {
      if (!detectingPeak && averageAdcValue > peakThreshold) {
        peaks.push_back(averageAdcValue);
        detectingPeak = true;
        Serial.print("Peak detected: ");
        Serial.println(averageAdcValue);
      } else if (detectingPeak && averageAdcValue < resetThreshold) {
        detectingPeak = false;
      }
    }
    
    int waiting_time = 1/(frequency*resolution*2)*1000; //Calcula o periodo esperado dado a frequência escolhida em milissegundos
    cycleEndTime = millis(); // Verifica o tempo que demorou nessa iteração
    while(cycleEndTime - cycleStartTime < waiting_time) { //Verifica se esse período ja ocorreu, se não ele continua preso no looping até dar o tempo
      cycleEndTime = millis();
    }
    dac_output_voltage(dacChannel, value >> 4); // Converte 12 bits para 8 bits, atualiza o valor na saída
    cycleStartTime = millis(); //Atualiza o valor de início da próxima iteração

  } else if (currentSystemMode == LOCK) {
    // Implementa o controle de feedback para travamento de cavidade

    //Calcula o novo valor a ser passado para o PZT
    value += direction*step;
    dac_output_voltage(dacChannel, value >> 4); // Converte 12 bits para 8 bits, atualiza o valor na saída

    //Verifica a resposta do sistema, faz uma média na leitura para filtrar ruídos de alta frequência
    long adcSum = 0;
    for (int i = 0; i < numReadings; i++) {
      adcSum += adc1_get_raw(adcChannel);
      delayMicroseconds(delayUs);  // Pequeno delay entre leituras para evitar leituras muito rápidas
    }
    int averageAdcValue = adcSum / numReadings;

    // Verifica a resposta do sistema
    if (averageAdcValue > lastAdcValue) {
      direction = direction;
    } else if ( averageAdcValue < lastAdcValue) {
      direction = -direction;
    }
    lastAdcValue = averageAdcValue;
  }
}