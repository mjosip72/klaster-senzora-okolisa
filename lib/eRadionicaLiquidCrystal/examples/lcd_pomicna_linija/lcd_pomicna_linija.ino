/*
Ispisuje pomicni tekst na LCD ekran.

Više o LCD-ovima u Croduino lekciji broj 12.
LCDovi: https://e-radionica.com/hr/moduli-aktuatori/lcd.html

2017. / e-radionica.com
techsupport@e-radionica.com
*/

#include <LiquidCrystal.h>

LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

int lcdSirina = 16;
int lcdVisina = 2;

byte logo[8] = { 
  B01110,
  B10001,
  B10111,
  B11101,
  B00111,
  B10001,
  B01110,
};

String gornjaLinija = "Ovo je tutorijal kako pomicati tekst samo jedne linije LCD-a..";
String donjaLinija = "e-radionica.com";

int nizPocetak, nizKraj = 0;
int pomicniKursor = lcdSirina;

void setup() 
{
  lcd.begin(lcdSirina, lcdVisina);
  lcd.createChar(0, logo);
}

void loop() 
{
  lcd.setCursor(pomicniKursor, 0);
  lcd.print(gornjaLinija.substring(nizPocetak, nizKraj));
  lcd.setCursor(0, 1);
  lcd.write(byte(0));
  lcd.print(donjaLinija);
  delay(300);
  
  lcd.clear();
  if (nizPocetak == 0 && pomicniKursor > 0)
  {
    pomicniKursor--;
    nizKraj++;
  } 
  
  else if (nizPocetak == nizKraj)  //bez ovog elsa scroll samo jednom
  {                                //ali treba postaviti tekst ponovno
   nizPocetak = nizKraj = 0;
   pomicniKursor = lcdSirina;
  } 
  
  else if (nizKraj == gornjaLinija.length() && pomicniKursor == 0) 
  {
    nizPocetak++;
  } 
  
  else 
  {
    nizPocetak++;
    nizKraj++;
  }
  
}
