#define VERSIONSTRING   "VENTTILLATER.git version 1.0" // 16 characters per line
#define SERIAL_BAUD_RATE        57600

#define CYCLES_PER_MINUTE_MIN       6
#define CYCLES_PER_MINUTE_MAX       25
#define INHALE_EXHALE_RATIO_MIN     1 // technically actually exhale/inhale ratio
#define INHALE_EXHALE_RATIO_MAX     5 // but this seems common terminology

#define TIME_FILL_MIN      100ul
#define TIME_FILL_MAX     5000ul

#define TIME_NEITHER_OPEN  100ul // time between closing one valve and opening another
#define TIME_VALVE_FULLCURRENT 50 // time to give solenoid full current

#define PWM_VALVE_GRAB  255 // assuming power source matches solenoid voltage this is 255
#define PWM_VALVE_HOLD  100 // PWM value to keep solenoid latched

#define KNOB_FILL                   A0
#define KNOB_CYCLES_PER_MINUTE      A1
#define KNOB_INHALE_EXHALE_RATIO    A2
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
  setSolenoidState(VALVE_FILL, (cycle_time < time_fill)
                            && (cycle_time < time_exhale)); // set fill valve
  // FILL VALVE WILL CLOSE WHEN EXHALE ENDS REGARDLESS OF FILL TIME SETTING

  setSolenoidState(VALVE_EXHALE, cycle_time < time_exhale); // set exhale valve

  setSolenoidState(VALVE_INHALE, (cycle_time > time_exhale + TIME_NEITHER_OPEN)
                              && (cycle_time < (time_inhale + time_exhale + TIME_NEITHER_OPEN))); // set inhale valve

  if (cycle_time > (time_inhale + time_exhale + (TIME_NEITHER_OPEN * 2))) {
    time_start_ventilate = millis(); // record the present time and restart the cycle
    Serial.print("Restart time_start_ventilate at "+String(time_start_ventilate));
    Serial.println("	time_fill:"+String(time_fill)+"	time_inhale:"+String(time_inhale)+"	time_exhale:"+String(time_exhale));
  }
}

void checkKnobs() {
  time_fill = (TIME_FILL_MIN + ((TIME_FILL_MAX - TIME_FILL_MIN)*knobRead(KNOB_FILL))/1023);

  uint32_t cycles_per_min = (CYCLES_PER_MINUTE_MIN + ((CYCLES_PER_MINUTE_MAX - CYCLES_PER_MINUTE_MIN)*knobRead(KNOB_CYCLES_PER_MINUTE))/1023);
  uint32_t cycle_duration = 60000ul / cycles_per_min;
  float inhale_exhale_denominator = (INHALE_EXHALE_RATIO_MIN + ((INHALE_EXHALE_RATIO_MAX - INHALE_EXHALE_RATIO_MIN)*knobRead(KNOB_INHALE_EXHALE_RATIO))/1023);

  time_inhale = cycle_duration / (1.0f + inhale_exhale_denominator);
  time_exhale = cycle_duration - time_inhale;
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

void setSolenoidState(uint16_t pinNumber, bool setState) {
  static uint8_t pinPwmValue[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}; // store the state of the pin
  if (setState && (pinPwmValue[pinNumber] == 0)) { // if pin was off
    Serial.print("pin "+String(pinNumber)+" OPEN	");
    digitalWrite(pinNumber, HIGH); // analogWrite(pinNumber, PWM_VALVE_GRAB);
    delay(TIME_VALVE_FULLCURRENT);
    //analogWrite(pinNumber, PWM_VALVE_HOLD);
    pinPwmValue[pinNumber] = PWM_VALVE_HOLD; // store pwm value
  } else if ((! setState) && (pinPwmValue[pinNumber] > 0)) {
    Serial.print("pin "+String(pinNumber)+" CLOSE	");
    digitalWrite(pinNumber, LOW);
    pinPwmValue[pinNumber] = 0; // store pwm value
  }
}

void setup() {
  pinMode(VALVE_FILL  ,OUTPUT);
  pinMode(VALVE_INHALE,OUTPUT);
  pinMode(VALVE_EXHALE,OUTPUT);
  pinMode(BEEPER_PIN  ,OUTPUT);

  setPwmFrequency(5, 8); // set PWM frequency for pins 5 and 6 to 31250Hz
  setPwmFrequency(9, 8); // set PWM frequency for pins 9 and 10 to 31250Hz

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

void setPwmFrequency(int pin, int divisor) {
  byte mode;
  if(pin == 5 || pin == 6 || pin == 9 || pin == 10) {
    switch(divisor) {
    case 1:
      mode = 0x01;
      break;
    case 8:
      mode = 0x02;
      break;
    case 64:
      mode = 0x03;
      break;
    case 256:
      mode = 0x04;
      break;
    case 1024:
      mode = 0x05;
      break;
    default:
      return;
    }
    if(pin == 5 || pin == 6) {
      TCCR0B = TCCR0B & 0b11111000 | mode;
    }
    else {
      TCCR1B = TCCR1B & 0b11111000 | mode;
    }
  }
  else if(pin == 3 || pin == 11) {
    switch(divisor) {
    case 1:
      mode = 0x01;
      break;
    case 8:
      mode = 0x02;
      break;
    case 32:
      mode = 0x03;
      break;
    case 64:
      mode = 0x04;
      break;
    case 128:
      mode = 0x05;
      break;
    case 256:
      mode = 0x06;
      break;
    case 1024:
      mode = 0x7;
      break;
    default:
      return;
    }
    TCCR2B = TCCR2B & 0b11111000 | mode;
  }
}
