/*
Primjer koda u kojemu ispisujemo jednostavan tekst na LCD ekran uz pomoc IIC adaptera,
tj. koristeci IIC komunikaciju. 
Za vise info o IIC komunikaciji, pogledajte Croduino lekciju broj 12. 
Kontrast ekrana podesiti potenciometrom koji je na poledini adaptera.

IIC LCD adapter: https://e-radionica.com/hr/iic-lcd-adapter.html
LCD ekrani: https://e-radionica.com/hr/moduli-aktuatori/lcd.html

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
  lcd.home ();                   // idi na pocetak prve linije(0,0)
  lcd.print("Hello World!");     // ispisi tekst
  lcd.setCursor(0,1);            // pocetak drugog reda
  lcd.print("e-radionica.com");  // ispisi tekst
}

void loop()
{
  // loop
}