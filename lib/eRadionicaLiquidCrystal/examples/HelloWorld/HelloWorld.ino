/*
Primjer koda u kojemu ispisujemo jednostavan tekst na LCD ekran.

Više o LCD-ovima u Croduino lekciji broj 12.
LCDovi: https://e-radionica.com/hr/moduli-aktuatori/lcd.html

2017. / e-radionica.com
techsupport@e-radionica.com
*/

#include <Wire.h> 
#include <LiquidCrystal.h>

#define   KONTRAST_PIN    9 // Pin na kojemu nam je spojen Vo pin LCDa
#define   POZOSVJETLJENJE 7 // anoda(+) LEDice pozadinskog osvjetljenja. Sada
                           // mozemo upravljati pozadinskim osvjetljenjem. 
#define   KONTRAST     110 // ovo je varijabla za kontrast(0-255)
                             
// fukcija iz librarya koja kreira LCD objekt
LiquidCrystal lcd(12, 11, 5, 4, 3, 2, POZOSVJETLJENJE, POSITIVE ); 

void setup()
{

  // Reci da je pin za kontrast output te podesi kontrast
  pinMode(KONTRAST_PIN, OUTPUT);
  analogWrite (KONTRAST_PIN, KONTRAST);

  lcd.backlight();  // upali backlight
    

  lcd.begin(16,2);               // potrebno da bismo mogli koristiti LCD.
                                 // 16,2 oznacava dimenziju LCDa 
  lcd.backlight();               // upali pozadinsko osvjetljenje
  delay(1000);                    // pricekaj 1s
  lcd.noBacklight();             // ugasi pozadinsko osvjetljenje 
  delay(1000);                    // pricekaj 1s
  lcd.backlight();               // opet upali pozadinsko osvjetljenje
  
  lcd.print("LCD je ispravno");  // ispisi jednostavan tekst
  lcd.setCursor(0,1);            // postavi kursor na pocetak drugog reda
  lcd.print("spojen. Bravo!");   // ispisi joj teksta
  delay(2000);                   // pricekaj 2s kako bi se tekst stigao procitati

  lcd.clear();                   // obrisi sve napisano na LCDu
  lcd.home();                    // idi na pocetak prve linije(0,0)
  lcd.print("Hello World!");     // ispisi tekst
  lcd.setCursor(0,1);            // pocetak drugog reda
  lcd.print("e-radionica.com");  // ispisi tekst
}

void loop()
{
	// loop
}
