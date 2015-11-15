#include <avr/sleep.h>

// PIN2 --+-- 1MegaOhm ----- GND
//        |
//        |\  <- Switch
//        |
//        +----------------- VCC

void wake () {
  sleep_disable();
  detachInterrupt (0);
}

void setup () {
  Serial.begin(57600);
}

void up() {
  ADCSRA = 0;  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
  noInterrupts ();
  attachInterrupt (0, wake, FALLING);
  EIFR = bit (INTF0);
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  interrupts ();
  sleep_cpu ();
}

void down() {
  ADCSRA = 0;  
  set_sleep_mode (SLEEP_MODE_PWR_DOWN);  
  sleep_enable();
  noInterrupts ();
  attachInterrupt (0, wake, RISING);
  EIFR = bit (INTF0);
  MCUCR = bit (BODS) | bit (BODSE);
  MCUCR = bit (BODS); 
  interrupts ();
  sleep_cpu ();
}

int state;

void loop () {
  state = digitalRead(2);
  Serial.println(state);
  delay(1000);
  
  int test = digitalRead(2);
  if(state == test) {
    if(state == 1) {
      up();
    } else {
      down();
    }
  }
}