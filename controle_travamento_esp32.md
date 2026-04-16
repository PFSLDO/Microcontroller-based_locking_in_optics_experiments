# Controle de Travamento com ESP32 + DAC Interno e MCP4725

## 📌 Contexto

Este projeto tem como objetivo implementar um sistema de varredura e travamento de sinal utilizando um **ESP32**, combinando o uso de:

- DAC interno do ESP32 (8 bits)
- DAC externo MCP4725 (12 bits, via I2C)

A primeira versão funcional deste sistema encontra-se no diretório:

/trav

Essa versão foi desenvolvida por Beatriz Marioto, utilizando exclusivamente o DAC interno do ESP32.

---

## ⚙️ Problema da Solução Original

A solução original é funcional, porém apresenta limitações importantes durante a etapa de **travamento (lock)**:

- O DAC interno possui apenas **8 bits de resolução**
- Isso resulta em **passos discretos relativamente grandes**
- Próximo ao pico do sinal:
  - O valor ideal muitas vezes está entre dois níveis possíveis do DAC
  - O sistema fica **oscilando entre dois estados**
  - Isso impede uma estabilização precisa

Em resumo, a limitação de resolução compromete a qualidade do travamento.

---

## 💡 Primeira Abordagem Considerada

A ideia inicial para melhorar o sistema foi:

Substituir completamente o DAC interno pelo DAC externo MCP4725 (12 bits)

### Motivação:
- Aumentar a resolução
- Melhorar a precisão no travamento

### Problema encontrado:
- O MCP4725 se comunica via **I2C**
- A comunicação I2C apresenta **limitação de velocidade**
- O sistema exige atualizações rápidas durante a varredura

Resultado:  
Não é viável simplesmente reutilizar a mesma lógica da versão original utilizando apenas o DAC externo.

---

## 🧠 Nova Estratégia Proposta

Após análise, foi identificado que o sistema possui duas etapas distintas com requisitos diferentes:

### 1. 🔍 Varredura (Scan)
- Objetivo: localizar aproximadamente a posição do pico
- Requisitos:
  - Alta velocidade
  - Baixa necessidade de precisão

### 2. 🔒 Travamento (Lock)
- Objetivo: manter o sinal estabilizado no pico
- Requisitos:
  - Alta precisão
  - Atualizações mais refinadas

---

## 🚀 Solução Híbrida

A estratégia adotada é utilizar cada DAC onde ele é mais eficiente:

### Durante a varredura:
- Utilizar o DAC interno do ESP32 (8 bits)
- Vantagem:
  - Alta velocidade (sem I2C)
  - Suficiente para localização aproximada do pico

### Durante o travamento:
- Utilizar o DAC externo MCP4725 (12 bits)
- Vantagem:
  - Alta resolução
  - Melhor precisão no controle fino

---

## ⚠️ Desafios da Implementação

O uso do DAC externo deve ser feito com cautela devido ao custo da comunicação I2C:

- Evitar atualizações excessivas
- Minimizar escrita no DAC externo
- Possível uso de:
  - Estratégias de controle incremental
  - Deadband (zona morta)
  - Atualização apenas quando necessário

---

## 🎯 Objetivo do Projeto

Desenvolver uma nova versão do sistema que:

- Preserve a velocidade da varredura
- Aumente a precisão no travamento
- Utilize de forma eficiente:
  - DAC interno (rapidez)
  - DAC externo (precisão)

---

## 📁 Estrutura Esperada

/trav              → Versão original (DAC interno apenas)
/nova_versao       → Implementação híbrida proposta
/docs              → Documentação do projeto

---

## 👩‍💻 Créditos

- Versão original: Beatriz Marioto
- Evolução do projeto: (seu nome aqui)

---

## 📌 Observação Final

Este projeto não é apenas uma melhoria de hardware, mas uma mudança de estratégia de controle, explorando as vantagens específicas de cada componente para obter melhor desempenho global do sistema.
