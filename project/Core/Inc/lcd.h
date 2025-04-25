#ifndef INC_LCD_H_
#define INC_LCD_H_

#include "main.h"

// Mapeamento de pinos para STM32 Nucleo L412KB

#define LCD_RS_GPIO_Port GPIOA
#define LCD_RS_Pin GPIO_PIN_10

#define LCD_EN_GPIO_Port GPIOB
#define LCD_EN_Pin GPIO_PIN_3

#define LCD_D4_GPIO_Port GPIOB
#define LCD_D4_Pin GPIO_PIN_5

#define LCD_D5_GPIO_Port GPIOB
#define LCD_D5_Pin GPIO_PIN_4

#define LCD_D6_GPIO_Port GPIOB
#define LCD_D6_Pin GPIO_PIN_10

#define LCD_D7_GPIO_Port GPIOA
#define LCD_D7_Pin GPIO_PIN_8

void LCD_Init(void);
void LCD_SendCommand(uint8_t cmd);
void LCD_SendData(uint8_t data);
void LCD_SendString(char *str);
void LCD_SetCursor(uint8_t row, uint8_t col);

#endif /* INC_LCD_H_ */
