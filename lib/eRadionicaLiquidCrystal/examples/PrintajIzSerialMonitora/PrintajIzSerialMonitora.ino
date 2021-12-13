/*
Ispisuje tekst kojega upišete u Serial Monitoru na LCD ekran.

Više o LCD-ovima u Croduino lekciji broj 12.
LCDovi: https://e-radionica.com/hr/moduli-aktuatori/lcd.html

2017. / e-radionica.com
techsupport@e-radionica.com
*/

#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define   KONTRAST_PIN    9 // Pin na kojemu nam je spojen Vo pin LCDa
#define   POZOSVJETLJENJE 7 // anoda(+) LEDice pozadinskog osvjetljenja. Sada
                           // mozemo upravljati pozadinskim osvjetljenjem. 
#define   KONTRAST     110 // ovo je varijabla za kontrast(0-255)
                             
// fukcija iz librarya koja kreira LCD objekt
LiquidCrystal lcd(12, 11, 5, 4, 3, 2, POZOSVJETLJENJE, POSITIVE ); 

void setup()
{
  Serial.begin(9600); // zapocni serijsku komunikaciju
  
  // Reci da je pin za kontrast output te podesi kontrast
  pinMode(KONTRAST_PIN, OUTPUT);
  analogWrite (KONTRAST_PIN, KONTRAST);
  
  lcd.begin(16,2);               // potrebno da bismo mogli koristiti LCD.
                                 // 16,2 oznacava dimenziju LCDa 
  lcd.backlight();               // upali pozadinsko osvjetljenje
}

void loop()
{
  if (Serial.available())  // ako su podaci dostupni na serijskom portu
  {
    delay(100);  // napravi pauzu od 0,1s da svi podaci stignu
    lcd.clear(); // obrisi sve s ekrana
    while (Serial.available() > 0) // sve dok postoje podaci na serijskom
                                   // portu..
    {
      lcd.write(Serial.read());    // lcd.write(); ispisuje jedan po jedan znak(char, slovo)
                                   // - pa tako sve dok postoje - u ovom slučaju dok nisu svi
                                   // pročitani s serijskog porta.
    }
  }
}
