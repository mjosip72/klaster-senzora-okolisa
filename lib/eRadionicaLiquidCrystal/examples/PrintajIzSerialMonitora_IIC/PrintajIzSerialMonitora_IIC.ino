/*
Ispisuje tekst kojega upišete u Serial Monitoru na LCD ekran uz koristenje IIC adaptera. 
Kontrast ekrana podesiti potenciometrom koji je na poledini adaptera.

Više o IIC komunikaciji i LCD-ovima u Croduino lekciji broj 12.
IIC LCD adapter: https://e-radionica.com/hr/iic-lcd-adapter.html
LCDovi: https://e-radionica.com/hr/moduli-aktuatori/lcd.html

2017. / e-radionica.com
techsupport@e-radionica.com
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

// kreira LCD objekt. Ne mijenjati varijable jer su pinovi LCDa tim redoslijedom
// zalemljeni s adapterom na plocici. 
// prvi argument u zagradi je I2C adresa adaptera
// ako Vam ne radi s navedenom adresom, pokrenite I2C scanner
// prema tutorijalu te pronadite adresu svog adaptera
// https://e-radionica.com/hr/blog/2016/07/14/1064/
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

void setup()
{
  Serial.begin(9600);            // zapocni serijsku komunikaciju
  
  lcd.begin(16,2);               // potrebno da bismo mogli koristiti LCD.
                                 // 16,2 oznacava dimenziju LCDa 
  lcd.backlight();               // upali pozadinsko osvjetljenje
}

void loop()
{
  if (Serial.available())  // ako su podaci dostupni na serijskom portu
  {
    delay(100);  // napravi pauzu od 0,1s da svi podaci(bajtovi) stignu
    lcd.clear(); // obrisi sve s ekrana
    while (Serial.available() > 0) // sve dok postoje podaci(bajtovi) na serijskom
                                   // portu..
    {
      lcd.write(Serial.read());    // lcd.write(); ispisuje jedan po jedan znak(char, slovo)
                                   // - pa tako sve dok postoje - u ovom slučaju dok nisu svi
                                   // pročitani s serijskog porta.
    }
  }
}
