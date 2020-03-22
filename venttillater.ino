#define VERSIONSTRING   "VENTTILLATER.git version 1.0" // 16 characters per line
#define SERIAL_BAUD_RATE        57600

#define TIME_FILL_MIN   100ul
#define TIME_FILL_MAX   1000ul
#define TIME_INHALE_MIN   500ul
#define TIME_INHALE_MAX   1000ul
#define TIME_EXHALE_MIN   100ul
#define TIME_EXHALE_MAX   3000ul
#define TIME_NEITHER_OPEN       100ul // time between closing one valve and opening another

#define KNOB_FILL       A0
#define KNOB_INHALE     A1
#define KNOB_EXHALE     A2
#define OVERSAMPLES     25 // number of times analogRead() is repeated per read

#define VALVE_FILL      10
#define VALVE_INHALE    9
#define VALVE_EXHALE    6

#define BEEPER_PIN      7       // connect a speaker or beeper
#define BADTONE	        200	// bad tone frequency
#define GOODTONE	1000	// good tone frequency

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

uint32_t time_fill;	 // time to fill reservoir (concurrent with exhale)
uint32_t time_inhale;	 // time to inhale
uint32_t time_exhale;	 // time to exhale
uint32_t time_start_ventilate; // record the time at start of cycle

void ventilate() {
  uint32_t cycle_time = millis() - time_start_ventilate;
  digitalWrite(VALVE_FILL, (cycle_time < time_fill)
                        && (cycle_time < time_exhale)); // set fill valve
  // FILL VALVE WILL CLOSE WHEN EXHALE ENDS REGARDLESS OF FILL TIME SETTING

  digitalWrite(VALVE_EXHALE, cycle_time < time_exhale); // set exhale valve

  digitalWrite(VALVE_INHALE, (cycle_time > time_exhale + TIME_NEITHER_OPEN)
                          && (cycle_time < (time_inhale + time_exhale + TIME_NEITHER_OPEN))); // set inhale valve

  if (cycle_time > (time_inhale + time_exhale + (TIME_NEITHER_OPEN * 2))) {
    time_start_ventilate = millis(); // record the present time and restart the cycle
    Serial.print("Restart time_start_ventilate at "+String(time_start_ventilate));
    Serial.println("	time_fill:"+String(time_fill)+"	time_inhale:"+String(time_inhale)+"	time_exhale:"+String(time_exhale));
  }
}

void checkKnobs() {
  time_fill = (TIME_FILL_MIN + ((TIME_FILL_MAX - TIME_FILL_MIN)*knobRead(KNOB_FILL))/1023);
  time_inhale = (TIME_INHALE_MIN + ((TIME_INHALE_MAX - TIME_INHALE_MIN)*knobRead(KNOB_INHALE))/1023);
  time_exhale = (TIME_EXHALE_MIN + ((TIME_EXHALE_MAX - TIME_EXHALE_MIN)*knobRead(KNOB_EXHALE))/1023);
  //Serial.println(); // newline after three knobReads
}

uint16_t knobRead(uint16_t pinNumber) {
  uint32_t analogAdder = 0;
  for (uint16_t j=0; j<OVERSAMPLES; j++) analogAdder += analogRead(pinNumber);
  uint32_t averageRead = analogAdder / OVERSAMPLES;
  //Serial.print("	pin "+String(pinNumber)+": "+String(averageRead));
  return averageRead;
}

void updateLCD() {
  lcd.setCursor(0, 1);
  lcd.print(VERSIONSTRING);
  lcd.print(millis() / 1000);
  // display knob settings or something
}

void handleSerial() {
  if (Serial.available()) {
    char inChar = Serial.read();
    Serial.print(inChar); // echo back to user
    // serial command handler goes here
    while (Serial.available()) Serial.read(); // throw away whatever else is in the serial buffer
  }
}

void setup() {
  pinMode(VALVE_FILL  ,OUTPUT);
  pinMode(VALVE_INHALE,OUTPUT);
  pinMode(VALVE_EXHALE,OUTPUT);
  pinMode(BEEPER_PIN  ,OUTPUT);

  Serial.begin(SERIAL_BAUD_RATE); // for diag
  Serial.println(VERSIONSTRING);

  lcd.begin(LCD_COLUMNS, LCD_ROWS);
  lcd.print(VERSIONSTRING);

  tone(BEEPER_PIN, GOODTONE, 1000); // tone(pin, frequency, duration); NON blocking

  time_start_ventilate = millis(); // initialize cycle timer
}

void loop() {
  updateLCD();
  checkKnobs();
  ventilate();
  delay(50); // TODO should not need this
}
