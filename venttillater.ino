#include "knob.hpp"

#define VERSIONSTRING   "VENTTILLATER.git version 1.0" // 16 characters per line
#define SERIAL_BAUD_RATE            57600

#define TIME_NEITHER_OPEN           100ul // time between closing one valve and opening another
#define TIME_VALVE_FULLCURRENT      50 // time to give solenoid full current

#define PWM_VALVE_GRAB              255 // assuming power source matches solenoid voltage this is 255
#define PWM_VALVE_HOLD              100 // PWM value to keep solenoid latched

// Reservoir fill time knob
Knob time_fill_knob(A2, 100.0f, 5000.0f);

// Venitilation cycles per minute knob
Knob cycles_per_minute_knob(A1, 10.0f, 40.0f);

// Technically exhale/inhale ratio. But this seem common terminology.
Knob inhale_exhale_ratio_knob(A0, 1.0f, 5.0f);

#define VALVE_FILL              6
#define VALVE_INHALE_OPEN       9
#define VALVE_INHALE_CLOSE      10
#define VALVE_EXHALE_OPEN       10
#define VALVE_EXHALE_CLOSE      9

#define BEEPER_PIN              12    // connect a speaker or beeper
#define BADTONE	                1000	// bad tone frequency
#define GOODTONE	              2000	// good tone frequency

// LCD R/W pin to ground
// LCD VSS pin to ground
// LCD VCC pin to 5V
// 10K potentiometer pins: (GND)---(LCD_VO)---(+5v)
// ends to +5V and ground
// wiper to LCD VO pin (pin 3)
#define LCD_RS 		      2	
#define LCD_Enable 	    3	
#define LCD_D4 		      4	
#define LCD_D5 		      5	
#define LCD_D6 		      7	
#define LCD_D7 		      8	
#define LCD_COLUMNS     16 
#define LCD_ROWS 	      2

#include <LiquidCrystal.h>
LiquidCrystal lcd(LCD_RS, LCD_Enable, LCD_D4, LCD_D5, LCD_D6, LCD_D7);

uint32_t time_fill;	 // time to fill reservoir (concurrent with exhale)
uint32_t time_inhale;	 // time to inhale
uint32_t time_exhale;	 // time to exhale
uint32_t time_start_ventilate; // record the time at start of cycle

void ventilate() {
  uint32_t cycle_time = millis() - time_start_ventilate;
  
  setFillSolenoidState((cycle_time < time_fill)
                            && (cycle_time < time_exhale)); // set fill valve
  // FILL VALVE WILL CLOSE WHEN EXHALE ENDS REGARDLESS OF FILL TIME SETTING

  setInhaleValveState(cycle_time < time_exhale); // set exhale valve

  /*
  setExhaleValveState((cycle_time > time_exhale + TIME_NEITHER_OPEN)
                              && (cycle_time < (time_inhale + time_exhale + TIME_NEITHER_OPEN))); // set inhale valve
  */

  if (cycle_time > (time_inhale + time_exhale)){// + (TIME_NEITHER_OPEN * 2))) {

    // record the present time and restart the cycle
    time_start_ventilate = millis(); 
    
    Serial.println("Restart time_start_ventilate at "+String(time_start_ventilate));
    
    Serial.println("	time_fill:"+String(time_fill)+"	time_inhale:"+String(time_inhale)+"	time_exhale:"+String(time_exhale));
  }
}

void setFillSolenoidState(bool setState) {
  static uint8_t pwmValue = 0; // store the state of the pin
  if (setState && (pwmValue == 0)) { // if pin was off
    Serial.println("fill OPEN	");
    digitalWrite(VALVE_FILL, HIGH); // analogWrite(VALVE_FILL, PWM_VALVE_GRAB);
    //delay(TIME_VALVE_FULLCURRENT);
    //analogWrite(VALVE_FILL, PWM_VALVE_HOLD);
    pwmValue = PWM_VALVE_HOLD; // store pwm value
  } else if ((! setState) && (pwmValue > 0)) {
    Serial.println("fill CLOSE	");
    digitalWrite(VALVE_FILL, LOW);
    pwmValue = 0; // store pwm value
  }
}

void setInhaleValveState(bool setState) {
  static bool valveState = false; // store the state of the valve
  if (setState && (valveState == false)) { // if the valve was closed and we want it open
    Serial.println("inhale  OPEN	");
    digitalWrite(VALVE_INHALE_OPEN, HIGH);
    digitalWrite(VALVE_INHALE_CLOSE, LOW);
    valveState = true; // store valve state
  } else if ((! setState) && (valveState == true)) {
    Serial.println("inhale  CLOSE	");
    digitalWrite(VALVE_INHALE_CLOSE, HIGH);
    digitalWrite(VALVE_INHALE_OPEN, LOW);
    valveState = false; // store valve state
  }
}

void setExhaleValveState(bool setState) {
  static bool valveState = false; // store the state of the valve
  if (setState && (valveState == false)) { // if the valve was closed and we want it open
    Serial.println("exhale  OPEN	");
    digitalWrite(VALVE_EXHALE_OPEN, HIGH);
    digitalWrite(VALVE_EXHALE_CLOSE, LOW);
    valveState = true; // store valve state
  } else if ((! setState) && (valveState == true)) {
    Serial.println("exhale  CLOSE	");
    digitalWrite(VALVE_EXHALE_CLOSE, HIGH);
    digitalWrite(VALVE_EXHALE_OPEN, LOW);
    valveState = false; // store valve state
  }
}

// Check the knobs and update global state. This currently
// samples each knob multiple times and performs lots of
// floating point computation.
void checkKnobs() {
  float cycle_duration = 60000.0f / cycles_per_minute_knob.read();

  time_fill = time_fill_knob.read();
  time_inhale = cycle_duration / (1.0f + inhale_exhale_ratio_knob.read());
  time_exhale = cycle_duration - time_inhale;

  //Serial.println(); // newline after three knobReads
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
  pinMode(VALVE_INHALE_CLOSE,OUTPUT);
  digitalWrite(VALVE_INHALE_CLOSE, HIGH); // start valve in closed mode
  pinMode(VALVE_EXHALE_CLOSE,OUTPUT);
  digitalWrite(VALVE_EXHALE_CLOSE, HIGH); // start valve in closed mode
  pinMode(VALVE_INHALE_OPEN,OUTPUT);
  pinMode(VALVE_EXHALE_OPEN,OUTPUT);
  pinMode(BEEPER_PIN  ,OUTPUT);

  //setPwmFrequency(5, 8); // set PWM frequency for pins 5 and 6 to 31250Hz
  //setPwmFrequency(9, 8); // set PWM frequency for pins 9 and 10 to 31250Hz

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
