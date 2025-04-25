#include <LiquidCrystal.h>

// Pinos do LCD Shield
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

void setup() {
  lcd.begin(16, 2); // inicializa LCD 16x2
  lcd.setCursor(0, 0);
  lcd.print("Hello, STM32!");
  lcd.setCursor(0, 1);
  lcd.print("LCD Shield OK!");
}

void loop() {
  // não precisa de nada aqui
}
