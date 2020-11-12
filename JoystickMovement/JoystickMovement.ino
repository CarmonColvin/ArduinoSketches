//www.elegoo.com
//2016.12.9

/*
  LiquidCrystal Library - Hello World

 Demonstrates the use a 16x2 LCD display.  The LiquidCrystal
 library works with all LCD displays that are compatible with the
 Hitachi HD44780 driver. There are many of them out there, and you
 can usually tell them by the 16-pin interface.

 This sketch prints "Hello World!" to the LCD
 and shows the time.

  The circuit:
 * LCD RS pin to digital pin 7
 * LCD Enable pin to digital pin 8
 * LCD D4 pin to digital pin 9
 * LCD D5 pin to digital pin 10
 * LCD D6 pin to digital pin 11
 * LCD D7 pin to digital pin 12
 * LCD R/W pin to ground
 * LCD VSS pin to ground
 * LCD VCC pin to 5V
 * 10K resistor:
 * ends to +5V and ground
 * wiper to LCD VO pin (pin 3)

 Library originally added 18 Apr 2008
 by David A. Mellis
 library modified 5 Jul 2009
 by Limor Fried (http://www.ladyada.net)
 example added 9 Jul 2009
 by Tom Igoe
 modified 22 Nov 2010
 by Tom Igoe

 This example code is in the public domain.

 http://www.arduino.cc/en/Tutorial/LiquidCrystal
 */

// include the library code:
#include <LiquidCrystal.h>

// initialize the library with the numbers of the interface pins
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

#define LCD_LIGHT_PIN A4

// Arduino pin for joystick
const int SW_pin = 2; // digital pin connected to switch output
const int X_pin = 0; // analog pin connected to X output
const int Y_pin = 1; // analog pin connected to Y output

float timestamp = 0;

const String versionString = "v0.211";

const int Lcd_backlight_pin = 4; // analog pin controlling backlight

const float MM_calib = 251.0;// rough calibration of 1mm of travel on joystick.

unsigned long lastMetaData = 0;

const bool debug = true; 

int Static_X = 0;
int Static_Y = 0;

void setup() {
  // set up the LCD's number of columns and rows:
  lcd.begin(16, 2);

  // initialize joystick as an input.
  pinMode(SW_pin, INPUT);
  digitalWrite(SW_pin, HIGH);

   // Set the LCD display backlight pin as an output.
  pinMode(LCD_LIGHT_PIN, OUTPUT);

  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);

  // Turn on the LCD backlight.
  digitalWrite(LCD_LIGHT_PIN, HIGH);

   splashLcd();
   flashAck();   

  // Turn off the LCD backlight and display
  digitalWrite(LCD_LIGHT_PIN, LOW);
  lcd.noDisplay();

  if (debug) {
      Serial.begin(9600);
  }

  sampleStatic();
}

void sampleStatic(){
  delay(300);
  Static_X = analogRead(X_pin);
  Static_Y = analogRead(Y_pin);
}

void flashAck() {
  digitalWrite(LED_BUILTIN, HIGH); // ON
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW); // OFF
  delay(959);
  digitalWrite(LED_BUILTIN, HIGH); // ON
  delay(41);
  digitalWrite(LED_BUILTIN, LOW); // OFF
}

void splashLcd() {
  lcd.setCursor(0, 0);
  lcd.print("- Datum Garage -");
  lcd.setCursor(0, 1);
  lcd.print("Firmware: ");
  lcd.print(versionString);
}

void loop() {
  if (digitalRead(SW_pin) == 0) {
    if (digitalRead(LCD_LIGHT_PIN)){
      digitalWrite(LCD_LIGHT_PIN, !digitalRead(LCD_LIGHT_PIN));
      lcd.noDisplay();
    }
    else {
      digitalWrite(LCD_LIGHT_PIN, !digitalRead(LCD_LIGHT_PIN));
      lcd.display();
    }
    delay(500);
  }

  int relativeX = analogRead(X_pin) - Static_X;
  int relativeY = analogRead(Y_pin) - Static_Y;
  
  // set the cursor to column 0, line 1
  // (note: line 1 is the second row, since counting begins with 0):
  lcd.setCursor(0, 0);
  lcd.print("X-axis: ");
  lcd.print(relativeX / MM_calib, 3);
  lcd.print(" mm     ");

  lcd.setCursor(0, 1);
  lcd.print("Y-axis: ");
  lcd.print(relativeY / MM_calib, 3);
  lcd.print(" mm    ");

  if (debug)
  {
   if (lastMetaData == 0 || millis() - lastMetaData > 5000)
   {
     lastMetaData = millis();
     Serial.println("@NAME:Datum Garage NBP");
     Serial.println();
     Serial.print("@VERSION:");
     Serial.println(versionString);
     Serial.println();
   }
    
   Serial.print("*NBP1,UPDATEALL,");
   Serial.println(millis() / 1000.0, 3);
   Serial.print("\"Deflection-X\",\"mm\":");
   Serial.println(relativeX / MM_calib, 3);
   Serial.print("\"Deflection-Y\",\"mm\":");
   Serial.println(relativeY / MM_calib, 3);
   Serial.println("#");
   Serial.println();
  }

  delay(10);
}
