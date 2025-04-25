#include "lcd.h"

// Função auxiliar para enviar um nibble para o LCD
void LCD_SendNibble(uint8_t nibble)
{
    HAL_GPIO_WritePin(D4_GPIO_Port, D4_Pin, (nibble >> 0) & 0x01);
    HAL_GPIO_WritePin(D5_GPIO_Port, D5_Pin, (nibble >> 1) & 0x01);
    HAL_GPIO_WritePin(D6_GPIO_Port, D6_Pin, (nibble >> 2) & 0x01);
    HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, (nibble >> 3) & 0x01);
}

// Função para enviar um comando para o LCD
void LCD_SendCommand(uint8_t command)
{
    HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_RESET);  // RS = 0 para comando
    HAL_GPIO_WritePin(RW_GPIO_Port, RW_Pin, GPIO_PIN_RESET);  // RW = 0 para escrita
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);    // EN = 1 para enviar dado
    LCD_SendNibble(command >> 4);                              // Envia nibble alto
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);   // Desabilita EN
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);     // Habilita EN novamente
    LCD_SendNibble(command & 0x0F);                            // Envia nibble baixo
    HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);   // Desabilita EN
}

// Função para inicializar o LCD
void LCD_Init(void)
{
    HAL_Delay(20);  // Atraso para estabilizar LCD
    LCD_SendCommand(0x02);  // Modo de 4 bits
    LCD_SendCommand(0x28);  // 2 linhas, modo de 4 bits
    LCD_SendCommand(0x0C);  // Liga display, sem cursor
    LCD_SendCommand(0x06);  // Move o cursor para a direita
    LCD_SendCommand(0x01);  // Limpa o display
    HAL_Delay(2);  // Atraso para limpar a tela
}

// Função para escrever uma string no LCD
void LCD_WriteString(char* str)
{
    while (*str)
    {
        HAL_GPIO_WritePin(RS_GPIO_Port, RS_Pin, GPIO_PIN_SET); // RS = 1 para dados
        HAL_GPIO_WritePin(RW_GPIO_Port, RW_Pin, GPIO_PIN_RESET);  // RW = 0 para escrita
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);    // EN = 1 para enviar dado
        LCD_SendNibble(*str >> 4);                              // Envia nibble alto
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);   // Desabilita EN
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_SET);     // Habilita EN novamente
        LCD_SendNibble(*str & 0x0F);                            // Envia nibble baixo
        HAL_GPIO_WritePin(EN_GPIO_Port, EN_Pin, GPIO_PIN_RESET);   // Desabilita EN
        str++;
    }
}
