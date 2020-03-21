#define VERSIONSTRING   'VENTTILLATER.git version 1.0' // 16 characters per line
#define KNOB_FILL       A3
#define KNOB_INHALE     A4
#define KNOB_EXHALE     A5
#define VALVE_FILL      3
#define VALVE_INHALE    4
#define VALVE_EXHALE    5
#define BEEPER_PIN      6       // connect a speaker or beeper
#define LCD_RS 		12	// LCD R/W pin to ground
#define LCD_Enable 	11	// LCD VSS pin to ground
#define LCD_D4 		5	// LCD VCC pin to 5V
#define LCD_D5 		4	// 10K potentiometer pins: (GND)---(LCD_VO)---(+5v)
#define LCD_D6 		3	// ends to +5V and ground
#define LCD_D7 		2	// wiper to LCD VO pin (pin 3)
#define LCD_COLUMNS     16	// 
#define LCD_ROWS 	2	// 
#include <LiquidCrystal.h>
LiquidCrystal lcd(LCD_RS, LCD_Enable, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

uint32_t time_fill =    500; // time to fill reservoir
uint32_t time_inhale =  500; // time to 
uint32_t time_exhale =  500; // time to 

void setup() {
  Serial.begin(57600); // for diag
  pinMode(VALVE_FILL  ,OUTPUT);
  pinMode(VALVE_INHALE,OUTPUT);
  pinMode(VALVE_EXHALE,OUTPUT);
  pinMode(BEEPER_PIN  ,OUTPUT);
  Serial.println(VERSIONSTRING);
  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcd.print(VERSIONSTRING);
  tone(BEEPER_PIN, 440, 1000); // tone(pin, frequency, duration); blocking?
}

void loop() {
  lcd.setCursor(0, 1);
  lcd.print(VERSIONSTRING);
  lcd.print(millis() / 1000);
  Serial.println("A3:"+String(analogRead(A3))+"	A4:"+String(analogRead(A4))+"	A5:"+String(analogRead(A5)));
  if (Serial.available()) {
    char inChar = Serial.read();
  }
}
