#include <Wire.h>
#include <Adafruit_MCP4725.h>

Adafruit_MCP4725 dac;

const float frequency = 10.0;     // frequência desejada
const int points = 200;           // pontos por período
const int dacMax = 4095;

unsigned long previousMicros = 0;
unsigned long interval;

int indexPoint = 0;

void setup() {
  Wire.begin();
  Wire.setClock(400000);   // 400 kHz

  dac.begin(0x60);

  interval = 1000000.0 / (frequency * points);  
}

void loop() {
  unsigned long currentMicros = micros();

  if (currentMicros - previousMicros >= interval) {
    previousMicros = currentMicros;

    float phase = (float)indexPoint / points;

    // Onda triangular 0 → 4095 → 0
    int value;
    if (phase < 0.5) {
      value = phase * 2 * dacMax;
    } else {
      value = (1.0 - phase) * 2 * dacMax;
    }

    dac.setVoltage(value, false);

    indexPoint++;
    if (indexPoint >= points) {
      indexPoint = 0;
    }
  }
}