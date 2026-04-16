#include <driver/dac.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <vector>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

/////////////////////////////////////// CONFIGURAÇÕES DO DISPLAY OLED
#define LARGURA_OLED 128
#define ALTURA_OLED 64
#define RESET_OLED -1

Adafruit_SSD1306 display(LARGURA_OLED, ALTURA_OLED, &Wire, RESET_OLED);

/////////////////////////////////////// CONFIGURAÇÕES DO MCP4725
#define MCP4725_ADDR 0x60     // Endereço I2C padrão do MCP4725
#define MCP4725_MAX_VALUE 4095 // 12 bits = 4095

/////////////////////////////////////// CONSTANTES E DEFINIÇÕES
// DAC Interno: 8 bits (0-255) = 0-3.3V
// DAC Externo: 12 bits (0-4095) = 0-5V (tipicamente 0-3.3V com jumper apropriado)

const dac_channel_t dacChannel = DAC_CHANNEL_1;    // DAC interno (GPIO25)
const adc1_channel_t adcChannel = ADC1_CHANNEL_0;  // ADC (GPIO36)

// Parâmetros de SWEEP
int resolution = 232;        // Amplitude em 8 bits (DAC interno)
int resolutionmax = 255;     // Máximo de 8 bits
int delayUs = 20;            // Delay entre amostras
int numReadings = 10;        // Número de leituras para média
const float referenceVoltage = 3.3;
unsigned long cycleStartTime = 0;
unsigned long cycleEndTime = 0;
int frequency = 10;          // Frequência em Hz
float amp_step = referenceVoltage / resolutionmax;
int step = 1;                // Passo para travamento
int averageAdcValue = 0;
int lastAdcValue = 0;
float amp = amp_step * resolution;
int direction = 1;
float value = 0;             // Valor do DAC interno
int value_ext = 0;           // Valor do DAC externo (12 bits)
int delayInterrupt = 250;
double waiting_time = 1000000 / (frequency * resolution * 2);

// Parâmetros de detecção de picos
const int peakThreshold = 40;
const int resetThreshold = 20;
bool detectingPeak = false;
std::vector<unsigned int> peaks_place;  // Armazena posições de picos (8 bits)
int currentPeakIndex = 0;

// Pinos dos botões
const int buttonPin = 15;    // GPIO15 - Alterna opções
const int increasePin = 4;   // GPIO4 - Incrementa
const int decreasePin = 17;  // GPIO17 - Decrementa
const int modeSwitchPin = 2; // GPIO2 - Alterna SWEEP/LOCK

bool optionButton = false;
bool increaseButton = false;
bool decreaseButton = false;
bool modeButton = false;

// Enums dos modos
enum ModeSweep { AMPLITUDE, FREQUENCY };
ModeSweep currentModeSweep = AMPLITUDE;

enum ModeLock { STEP, PEAK, AMOSTRAS };
ModeLock currentModeLock = STEP;

enum SystemMode { SWEEP, LOCK };
SystemMode currentSystemMode = SWEEP;

int targetValue = 0;  // Valor do DAC interno onde o pico foi detectado

/////////////////////////////////////// FUNÇÕES DE I2C para MCP4725
void writeDAC_MCP4725(int value) {
  // Limita o valor a 12 bits
  if (value < 0) value = 0;
  if (value > MCP4725_MAX_VALUE) value = MCP4725_MAX_VALUE;

  // MCP4725 espera 2 bytes:
  // Byte 1: [0 PD1 PD0 D11 D10 D9 D8 D7]
  // Byte 2: [D6 D5 D4 D3 D2 D1 D0 x]
  // Modo normal: PD1=0, PD0=0

  Wire.beginTransmission(MCP4725_ADDR);
  Wire.write((value >> 8) & 0x0F);  // Bits [11:8] do valor
  Wire.write(value & 0xFF);          // Bits [7:0] do valor
  Wire.endTransmission();
}

/////////////////////////////////////// FUNÇÕES DE INTERRUPÇÃO DOS BOTÕES

void IRAM_ATTR handleButtonPin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    optionButton = true;
    if (currentSystemMode == SWEEP) {
      currentModeSweep = static_cast<ModeSweep>((currentModeSweep + 1) % 2);
    } else if (currentSystemMode == LOCK) {
      currentModeLock = static_cast<ModeLock>((currentModeLock + 1) % 3);
    }
  }
  lastInterruptTime = interruptTime;
}

void IRAM_ATTR handleIncreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    increaseButton = true;
    if (currentSystemMode == SWEEP) {
      switch (currentModeSweep) {
        case AMPLITUDE:
          resolution += 4;
          if (resolution > 255) resolution = 255;
          amp = amp_step * resolution;
          break;
        case FREQUENCY:
          frequency += 5;
          if (frequency > 50) frequency = 50;
          break;
      }
      waiting_time = 1000000 / (frequency * resolution * 2);
    } else if (currentSystemMode == LOCK) {
      switch (currentModeLock) {
        case PEAK:
          if (!peaks_place.empty()) {
            currentPeakIndex = (currentPeakIndex + 1) % peaks_place.size();
            targetValue = peaks_place[currentPeakIndex];
            // Converte posição de 8 bits para 12 bits (escala: 0-255 -> 0-4095)
            value_ext = (targetValue * MCP4725_MAX_VALUE) / 255;
          }
          break;
        case STEP:
          step++;
          if (step > 20) step = 20;
          break;
        case AMOSTRAS:
          numReadings++;
          if (numReadings > 30) numReadings = 30;
          break;
      }
    }
  }
  lastInterruptTime = interruptTime;
}

void IRAM_ATTR handleDecreasePin() {
  static unsigned long lastInterruptTime = 0;
  unsigned long interruptTime = millis();
  if (interruptTime - lastInterruptTime > delayInterrupt) {
    decreaseButton = true;
    if (currentSystemMode == SWEEP) {
      switch (currentModeSweep) {
        case AMPLITUDE:
          resolution -= 4;
          if (resolution < 12) resolution = 12;
          amp = amp_step * resolution;
          break;
        case FREQUENCY:
          frequency -= 5;
          if (frequency < 5) frequency = 5;
          break;
      }
      waiting_time = 1000000 / (frequency * resolution * 2);
    } else if (currentSystemMode == LOCK) {
      switch (currentModeLock) {
        case PEAK:
          if (!peaks_place.empty()) {
            currentPeakIndex = (currentPeakIndex - 1 + peaks_place.size()) % peaks_place.size();
            targetValue = peaks_place[currentPeakIndex];
            value_ext = (targetValue * MCP4725_MAX_VALUE) / 255;
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
    currentSystemMode = static_cast<SystemMode>((currentSystemMode + 1) % 2);
    switch (currentSystemMode) {
      case SWEEP:
        value = 0;
        direction = 1;
        dac_output_voltage(dacChannel, 0);
        break;
      case LOCK:
        if (!peaks_place.empty()) {
          targetValue = peaks_place[currentPeakIndex];
          value_ext = (targetValue * MCP4725_MAX_VALUE) / 255;
          writeDAC_MCP4725(value_ext);
        }
        lastAdcValue = averageAdcValue - 1;
        direction = 1;
        break;
    }
  }
  lastInterruptTime = interruptTime;
}

/////////////////////////////////////// FUNÇÕES AUXILIARES DE DISPLAY

void updateDisplaySweep() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(15, 8);
  display.print("Modo:Varredura");
  display.drawLine(15, 17, 15 + 14 * 6, 17, WHITE);
  display.setCursor(15, 30);
  display.print("Amplitude:");
  display.print(amp);
  display.print("V");
  display.setCursor(15, 50);
  display.print("Frequencia:");
  display.print(frequency);
  display.print("Hz");
  if (currentModeSweep == AMPLITUDE) {
    display.setCursor(0, 30);
  } else {
    display.setCursor(0, 50);
  }
  display.write(62);
  display.display();
}

void updateDisplayLock() {
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.clearDisplay();
  display.setCursor(15, 8);
  display.print("Modo:Travamento");
  display.drawLine(15, 17, 15 + 15 * 6, 17, WHITE);
  display.setCursor(15, 20);
  display.print("Step:");
  display.print(step);
  display.setCursor(15, 32);
  display.print("Pico:");
  if (!peaks_place.empty()) {
    display.print(currentPeakIndex + 1);
  } else {
    display.print("-");
  }
  display.setCursor(15, 44);
  display.print("Amostras:");
  display.print(numReadings);

  switch (currentModeLock) {
    case STEP:
      display.setCursor(0, 20);
      break;
    case PEAK:
      display.setCursor(0, 32);
      break;
    case AMOSTRAS:
      display.setCursor(0, 44);
      break;
  }
  display.write(62);
  display.display();
}

/////////////////////////////////////// SETUP
void setup() {
  Serial.begin(115200);

  // Inicializa I2C para MCP4725
  Wire.begin();
  Wire.setClock(400000);  // 400kHz I2C clock

  // Configura display OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("Falha ao inicializar display SSD1306"));
    while (1);
  }
  updateDisplaySweep();

  // Configura DAC interno
  dac_output_enable(dacChannel);
  dac_output_voltage(dacChannel, 0);

  // Configura ADC
  adc1_config_width(ADC_WIDTH_BIT_12);
  adc1_config_channel_atten(adcChannel, ADC_ATTEN_DB_0);

  // Configura botões
  pinMode(increasePin, INPUT_PULLUP);
  pinMode(decreasePin, INPUT_PULLUP);
  pinMode(modeSwitchPin, INPUT_PULLUP);
  pinMode(buttonPin, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(buttonPin), handleButtonPin, FALLING);
  attachInterrupt(digitalPinToInterrupt(increasePin), handleIncreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(decreasePin), handleDecreasePin, FALLING);
  attachInterrupt(digitalPinToInterrupt(modeSwitchPin), handleModeSwitchPin, FALLING);

  // Inicializa DAC externo
  writeDAC_MCP4725(0);

  Serial.println("Sistema inicializado!");
}

/////////////////////////////////////// LOOP PRINCIPAL
void loop() {
  if (currentSystemMode == SWEEP) {
    // ============ MODO SWEEP ============
    // Usa apenas DAC interno para varredura rápida

    cycleStartTime = micros();

    // Gera sinal triangular
    value += direction;
    if (value >= resolution) {
      value = resolution;
      direction = -1;
    } else if (value <= 0) {
      value = 0;
      direction = 1;
    }

    // Lê ADC
    averageAdcValue = adc1_get_raw(adcChannel);

    // Detecta picos (apenas na subida)
    if (direction == 1) {
      if (averageAdcValue > peakThreshold) {
        if (detectingPeak) {
          peaks_place.pop_back();
        }
        peaks_place.push_back(value);
        detectingPeak = true;
      }
    }
    if (detectingPeak && averageAdcValue < resetThreshold) {
      detectingPeak = false;
    }

    // Atualiza display conforme botões pressionados
    if (increaseButton == true) {
      increaseButton = false;
      updateDisplaySweep();
    }
    if (decreaseButton == true) {
      decreaseButton = false;
      updateDisplaySweep();
    }
    if (optionButton == true) {
      optionButton = false;
      updateDisplaySweep();
    }
    if (modeButton == true) {
      modeButton = false;
      updateDisplayLock();
    }

    // Aguarda tempo apropriado para manter frequência
    cycleEndTime = micros();
    while (cycleEndTime - cycleStartTime < waiting_time) {
      cycleEndTime = micros();
    }

    // Atualiza DAC interno
    dac_output_voltage(dacChannel, (int)value);

  } else if (currentSystemMode == LOCK) {
    // ============ MODO LOCK ============
    // Usa DAC externo para maior precisão

    // Calcula novo valor (incrementa/decrementa em 12 bits)
    value_ext += direction * step * 16;  // 16 = escala para 12 bits (approx)
    if (value_ext < 0) value_ext = 0;
    if (value_ext > MCP4725_MAX_VALUE) value_ext = MCP4725_MAX_VALUE;

    // Atualiza DAC externo (CUIDADO: I2C é lento, evita atualização muito rápida)
    writeDAC_MCP4725(value_ext);

    // Lê ADC com múltiplas amostras para filtering
    long adcSum = 0;
    for (int i = 0; i < numReadings; i++) {
      adcSum += adc1_get_raw(adcChannel);
      delayMicroseconds(delayUs);
    }
    averageAdcValue = adcSum / numReadings;

    // Lógica de rastreamento: segue o pico
    if (averageAdcValue > lastAdcValue) {
      direction = direction;  // Continua na mesma direção
    } else if (averageAdcValue < lastAdcValue) {
      direction = -direction;  // Inverte direção
    }

    lastAdcValue = averageAdcValue;

    // Atualiza display conforme botões
    if (increaseButton == true) {
      increaseButton = false;
      updateDisplayLock();
    }
    if (decreaseButton == true) {
      decreaseButton = false;
      updateDisplayLock();
    }
    if (optionButton == true) {
      optionButton = false;
      updateDisplayLock();
    }
    if (modeButton == true) {
      modeButton = false;
      updateDisplaySweep();
    }
  }
}
