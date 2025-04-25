#include "lcd.h"

int main(void)
{
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    // MX_TIMx_Init(); se precisar de timer

    LCD_Init();
    LCD_SetCursor(0, 0);
    LCD_SendString("Hello, STM32!");
    LCD_SetCursor(1, 0);
    LCD_SendString("LCD Shield OK!");

    while (1)
    {
        // pode fazer outra lógica aqui
    }
}
