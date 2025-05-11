//Programa: Teste LCD 16x2 com Keypad
//Autor: Arduino e Cia
  
#include <LiquidCrystal.h>  
  
LiquidCrystal lcd(8, 9, 4, 5, 6, 7);  
  
void setup()   
{  
 lcd.begin(16, 2);  
 lcd.setCursor(0,0);  
 lcd.print("Arduino e Cia");  
 lcd.setCursor(0,1);  
 lcd.print("Tecla :");  
}  
  
void loop()  
{  
 int botao;  
 botao = analogRead (0);  //Leitura do valor da porta analógica A0
 lcd.setCursor(8,1);  
 if (botao < 100) {  
  lcd.print ("Direita ");  
 }  
 else if (botao < 200) {  
  lcd.print ("Cima    ");  
 }  
 else if (botao < 400){  
  lcd.print ("Baixo   ");  
 }  
 else if (botao < 600){  
  lcd.print ("Esquerda");  
 }  
 else if (botao < 800){  
  lcd.print ("Select  ");  
 }  
}