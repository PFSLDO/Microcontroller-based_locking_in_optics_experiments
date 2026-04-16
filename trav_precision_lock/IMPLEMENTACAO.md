# 🔒 Sistema de Travamento Híbrido com Precisão - Implementação

## 📋 Índice
1. [Visão Geral da Solução](#visão-geral)
2. [Componentes Necessários](#componentes)
3. [Diagrama de Conexões](#diagrama)
4. [Detalhamento de Pinos](#pinos)
5. [Configurações de Hardware](#configuração)
6. [Notas Importantes](#notas)

---

## 🎯 Visão Geral da Solução {#visão-geral}

Este sistema implementa uma **estratégia híbrida de varredura e travamento** utilizando dois DACs:

### Etapa 1: Varredura (Scan)
- ⚡ **Rápida e eficiente**
- 🎯 Usa **DAC interno do ESP32 (8 bits)**
- 📊 Detecta automaticamente picos do sinal
- ❌ Sem comunicação I2C (nenhum overhead)

### Etapa 2: Travamento (Lock)
- 🎚️ **Preciso e estável**
- 🎯 Usa **DAC externo MCP4725 (12 bits)**
- 🔄 Rastreia o pico com algoritmo hill-climbing
- ✅ Comunicação I2C conforme necessário

### Vantagens
| Aspecto | Varredura | Travamento |
|---------|-----------|-----------|
| **Velocidade** | Máxima (DAC interno) | Controlada (I2C) |
| **Resolução** | 8 bits (suficiente) | 12 bits (precisa) |
| **Consumo** | Baixo | Moderado |
| **Oscilação** | Aceitável | Minimizada |

---

## 🔧 Componentes Necessários {#componentes}

### Microcontrolador
- **ESP32 NodeMCU WROOM32 WiFi**
  - DAC interno: 2 canais (GPIO25/P25, GPIO26/P26)
  - ADC: Múltiplos canais (GPIO36/P36, GPIO39/P39, etc.)
  - I2C: Integrado (GPIO21/P21=SDA, GPIO22/P22=SCL)

### Display
- **LCD 20x4 (HD44780 compatível)**
  - Comunicação: Paralela (4 bits)
  - Tensão: 5V ou 3.3V dependendo do módulo
  - Interfaces usadas: RS, E, D4, D5, D6, D7

### DAC Externo
- **MCP4725 (Adafruit ou genérico)**
  - Resolução: 12 bits (0-4095)
  - Comunicação: I2C
  - Tensão saída: 0-5V (configurável com jumper)
  - Endereço I2C padrão: 0x60

### Detector de Sinal
- **Fotodetector com amplificador** (retorna sinal analógico 0-3.3V)

### Acessórios
- **Cristal de PZT** (piezoelétrico com tensão 0-3.3V para varredura)
- **4 Botões momentâneos** (com pull-up interno)
- **Capacitores de desacoplamento** (100nF no VCC de cada módulo)
- **Resistores pull-up I2C** (4.7kΩ opcionais, se o módulo não tiver)

---

## 📐 Diagrama de Conexões {#diagrama}

### Visão Geral do Sistema

```
┌─────────────────────────────────────────────────────────────────┐
│                    ESP32 NodeMCU WROOM32                        │
│  ┌──────────────────────────────────────────────────────────┐   │
│  │                    Controle e Processamento              │   │
│  └──────────────────────────────────────────────────────────┘   │
└─────┬───────────┬───────────┬───────────┬───────────┬───────────┘
      │           │           │           │           │
   D25 (DAC1)  P36 (ADC)    I2C (P21/P22)  GPIO (Botões)  PZT
      │           │           │           │           │
      │           │        ┌──┴────┐      │           │
      │           │        │       │      │           │
      ▼           ▼        ▼       ▼      ▼           ▼
   ┌────┐    ┌────────┐ ┌────┐ ┌────┐ ┌─────┐   ┌─────────┐
   │DAC│    │ ADC    │ │LCD │ │MCP │ │BTN  │   │  PZT    │
   │INT│    │FOTO    │ │    │ │4725│ │ x4  │   │Control  │
   └────┘    └────────┘ └────┘ └────┘ └─────┘   └─────────┘
      │           │        │       │
      └─────────┬─┼────────┼───────┘
            VCC │ │  GND   │
                │ │        │
                150-      Black
            Com   50V
           Control(opt)
```

### Esquema Detalhado de Pinos

```
ESP32 NodeMCU WROOM32 PIN ASSIGNMENT
═══════════════════════════════════════════

[GPIO25 (P25)] ──────────► DAC1 (8 bits para PZT)
[GPIO26 (P26)] ──────────► DAC2 (não usado neste projeto)

[GPIO36 (P36)] ──────────► ADC1_CH0 (sinal do fotodetector)
(Note: GPIO36=ADC1_CHANNEL_0, requer atenuação AD DB_0)

[GPIO21 (P21)] ──────────┐
                        ├─ I2C (400kHz)
[GPIO22 (P22)] ──────────┤
                        └─► MCP4725 (endereço 0x60)

[GPIO12 (P12)] ──────────► LCD RS
[GPIO14 (P14)] ──────────► LCD EN
[GPIO13 (P13)] ──────────► LCD D4
[GPIO27 (P27)] ──────────► LCD D5
[GPIO26 (P26)] ──────────► LCD D6
[GPIO16 (P16)] ──────────► LCD D7

[GPIO15 (P15)] ──────────► BOTÃO 1 (Pull-up interno)
                          └─ Alterna opções (AMPLITUDE ↔ FREQUENCY em SWEEP)

[GPIO4 (P4)]  ──────────► BOTÃO 2 (Pull-up interno)
                          └─ Incrementa (+)

[GPIO17 (P17)] ──────────► BOTÃO 3 (Pull-up interno)
                          └─ Decrementa (-)

[GPIO2 (P2)]  ──────────► BOTÃO 4 (Pull-up interno)
                          └─ Alterna modo (SWEEP ↔ LOCK)

[GND]   ──────────► Malha comum (todos os GNDs)

[5V ou 3.3V] ─────► Alimentação conforme componentes
```

---

## 🔌 Conexões Físicas Detalhadas {#pinos}

### 1️⃣ Cristal de PZT (para Varredura)

```
ESP32 GPIO25 (P25)
        │
        R 100Ω (opcional, proteção)
        │
        ┣━━━━━━━ PZT (+)
        │
       GND ─────────────── PZT (-)
```

**Notas:**
- Tensão: 0-3.3V (controlada por GPIO25/P25, 0-255 em 8 bits)
- Para maior potência, usar amplificador de voltagem externo
- Recomendado: Resistor de proteção 100Ω

---

### 2️⃣ Fotodetector (Entrada do Sinal)

```
Fotodetector (Amplificador saída)
        │
        0-3.3V (sinal analógico)
        │
ESP32 GPIO36 (P36)
        │
       GND ─────────────── Fotodetector GND
```

**Notas:**
- ADC configurado em 12 bits (0-4095)
- Sem atenuação (ADC_ATTEN_DB_0) → máximo 1.2V
- Usar atenuação se sinal > 1.2V (ex: ADC_ATTEN_DB_11 para 0-3.3V)
- Filtro capacitivo (~10nF) entre sinal e GND recomendado

---

### 3️⃣ Display LCD 20x4

```
LCD 20x4 (HD44780 compatível)
═══════════════════════════════════
  PIN 1 [VSS]  ───┬──────────► GND
  PIN 2 [VDD]  ───┬──────────► 5V (ou 3.3V se suportado)
  PIN 3 [VO]   ───┬──────────► Contraste (potenciômetro de 10k)
  PIN 4 [RS]   ───┬──────────► ESP32 GPIO12 (P12)
  PIN 5 [RW]   ───┬──────────► GND
  PIN 6 [E]    ───┬──────────► ESP32 GPIO14 (P14)
  PIN 11[D4]  ───┬──────────► ESP32 GPIO13 (P13)
  PIN 12[D5]  ───┬──────────► ESP32 GPIO27 (P27)
  PIN 13[D6]  ───┬──────────► ESP32 GPIO26 (P26)
  PIN 14[D7]  ───┬──────────► ESP32 GPIO16 (P16)

  ┌─ Capacitor 100nF entre VCC e GND (próximo ao módulo)
```

**Notas:**
- Comunicação paralela 4 bits
- Display mostra:
  - Modo atual (SWEEP ou LOCK)
  - Parâmetros ajustáveis (Amplitude, Frequência, Step, Picos)
  - Indicador de opção selecionada (">")

---

### 4️⃣ DAC Externo MCP4725

```
MCP4725 (12 bits, via I2C)
═══════════════════════════
  PIN 1 [VCC]  ───┬──────────► 3.3V (ou 5V)
  PIN 2 [GND]  ───┬──────────► GND
  PIN 3 [SCL]  ───┼──────────► ESP32 GPIO22 (P22) (I2C Clock)
  PIN 4 [SDA]  ───┼──────────► ESP32 GPIO21 (P21) (I2C Data)
  PIN 5 [A0]   ───┘──────────► GND (endereço 0x60)
                             ou 3.3V (endereço 0x61)
  PIN 6 [OUT]  ───────────────► Amplificador/Comparador PZT
  PIN 7 [GND]  ───────────────► GND
  
  Endereço I2C: 0x60 (se A0 conectado em GND)
  
  ┌─ Capacitor 100nF entre VCC e GND (próximo ao módulo)
```

**Notas:**
- Saída: 0-5V (12 bits = 0-4095)
- Usada SOMENTE no modo LOCK (economia de I2C)
- Jumper de seleção de faixa de saída (0-5V typical, verificar módulo)
- Clock I2C: 400kHz (velocidade máxima segura)

---

### 5️⃣ Botões de Controle

```
Todos os botões usam PULL-UP INTERNO do ESP32

BOTÃO 1 (GPIO15/P15)      BOTÃO 2 (GPIO4/P4)       BOTÃO 3 (GPIO17/P17)    BOTÃO 4 (GPIO2/P2)
    │                        │                         │                      │
   3.3V                     3.3V                      3.3V                   3.3V
    │                        │                         │                      │
    ├─ [SW] ─┐            ├─ [SW] ─┐               ├─ [SW] ─┐           ├─ [SW] ─┐
    │        │            │        │               │        │           │        │
   GND       └────────────► GPIO15 (P15)         GND       │           │        │
                                                          └───────────► GPIO4 (P4)
               GND                                   GND              │
                                                                └───────────► GPIO17 (P17)
                                                           GND          GND
                                                                      └────────► GPIO2 (P2)
                                                                       GND
```

**Configuração:**
```cpp
pinMode(15, INPUT_PULLUP);  // BOTÃO 1: Alterna opções
pinMode(4,  INPUT_PULLUP);  // BOTÃO 2: Incrementa
pinMode(17, INPUT_PULLUP);  // BOTÃO 3: Decrementa
pinMode(2,  INPUT_PULLUP);  // BOTÃO 4: Modo SWEEP ↔ LOCK
```

**Notas:**
- Debounce via software (250ms delays)
- Interrupções em FALLING edge
- Capacitor ~10nF opcional entre sinal e GND (anti-ruído)

---

## ⚙️ Configurações de Hardware {#configuração}

### Conversão de Escalas

#### DAC Interno (8 bits) → Voltagem
```
Voltagem = (valor_8bits / 255) × 3.3V
Exemplo: valor=128 → (128/255)×3.3V ≈ 1.65V
```

#### Posição de Pico (8 bits) → DAC Externo (12 bits)
```
valor_12bits = (valor_8bits / 255) × 4095
Exemplo: pico em 128 (8 bits) → (128/255)×4095 ≈ 2048 (12 bits)
```

#### Step em Modo LOCK
```
incremento_12bits = direction × step × 16
Onde step varia de 1 a 20 (ajustável via botão)
```

### Limites de Frequência

```
Frequência (Hz) | Amplitude | Tempo por ciclo
─────────────────────────────────────────────
      5         |    255    |  200 ms
     10         |    232    |   87 ms
     20         |    200    |   42 ms
     30         |    150    |   22 ms
     50         |    100    |    8 ms

Fórmula: waiting_time (μs) = 1,000,000 / (freq × resolution × 2)
```

### Timing do Modo LOCK

```
Velocidade de I2C:        400 kHz
Tempo de escrita MCP4725: ~1-2 ms
Tempo de leitura ADC:     ~1-3 μs por amostra
Tempo total por ciclo:    ~1-2 ms + tempo de processamento
Taxa de atualização:      ~500-1000 Hz (com múltiplas amostras)
```

---

## ⚠️ Notas Importantes {#notas}

### 1. Alimentação
- ✅ **ESP32**: USB 5V ou bateria 3.7V (com regulador)
- ✅ **LCD 20x4**: 5V ou 3.3V (usar contraste apropriado)
- ✅ **MCP4725**: 3.3V ou 5V (verificar jumper do módulo)
- ⚠️ **Fotodetector/PZT**: Depende do amplificador externo

### 2. Comunicação I2C
```cpp
Wire.begin(21, 22);        // SDA=GPIO21 (P21), SCL=GPIO22 (P22)
Wire.setClock(400000);     // 400kHz para máxima velocidade
```

### 3. Proteção de Picos
- Adicionar capacitor ~100nF entre VCC e GND em cada módulo I2C
- Resistores pull-up I2C: 4.7kΩ (geralmente já inclusos nos módulos)

### 4. Otimizações para Modo LOCK
- ❌ **Evitar**: Lertura muito frequente do MCP4725
- ✅ **Fazer**: Usar variável local `value_ext` e atualizar I2C uma vez por ciclo
- ✅ **Considerar**: Deadband para evitar oscilações (não implementado por padrão)

### 5. Calibração Recomendada
```
1. Ajustar threshold de detecção: peakThreshold = 40 (variável)
2. Testar com voltagem de saída conhecida (multímetro)
3. Validar leitura de ADC com resistor divisor padrão
4. Medir jitter do sinal PZT em modo LOCK
```

### 6. Detecção de Problemas

| Problema | Causa Provável | Solução |
|----------|---|---|
| Display não aparece | Conexão paralela incorreta ou alimentação errada | Verificar pinos RS/E/D4-D7 (P12/P14/P13/P27/P26/P16), RW/GND, VCC e contraste |
| DAC externo não responde | Endereço wrong (0x60?) | Variar A0 (GND ou 3.3V), testar com scanner I2C em P21/P22 |
| Leitura ADC errada | Atenuação insuficiente | Aumentar ADC_ATTEN_DB_11 se > 1.2V no pino P36 |
| Oscilação em LOCK | Step muito grande | Reduzir step (botão -) |
| PZT não responde em SWEEP | DAC desabilitado | Verificar `dac_output_enable()` no pino P25 |
| Botões não respondem | Pull-up não funciona | Testar GPIO direto com Serial nos pinos P15/P4/P17/P2 |

---

## 📚 Referências Rápidas

### Includes Necessários
```cpp
#include <driver/dac.h>           // DAC interno
#include <driver/adc.h>           // ADC
#include <esp_adc_cal.h>          // Calibração ADC
#include <Wire.h>                 // I2C para MCP4725
#include <LiquidCrystal.h>        // Display LCD 20x4
```

### Links Úteis
- 📖 [ESP32 Pinout Reference](https://randomnerdtutorials.com/esp32-pinout-reference-Which-GPIO-pins-should-you-use/)
- 📖 [MCP4725 Datasheet](https://ww1.microchip.com/en-us/product/MCP4725)
- 📖 [LiquidCrystal Arduino Library](https://www.arduino.cc/reference/en/libraries/liquidcrystal/)

---

## 🔍 Próximos Passos

1. ✅ Montar circuito conforme diagrama
2. ✅ Verificar todas as conexões I2C (scanner I2C)
3. ✅ Testar DAC interno (valores 0-255)
4. ✅ Testar DAC externo (valores 0-4095)
5. ✅ Calibrar ADC com sinal conhecido
6. ✅ Validar detecção de picos em modo SWEEP
7. ✅ Validar rastreamento em modo LOCK
8. ✅ Otimizar parâmetros (step, threshold, frequency)

---

**Versão**: 1.0  
**Data**: Abril 2026  
**Autor**: Beatriz Marioto + Evolução do projeto
