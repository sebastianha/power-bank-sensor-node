#include <stdint.h>
#include <EEPROM.h>
#include <VirtualWire.h>
#include <avr/sleep.h>

// PIN2 --+-- 1MegaOhm ----- GND
//        |
//        |\  <- Switch
//        |
//        +----------------- VCC

#define RANDOM_PIN              1      // Needs to be an unconnected ANALOG pin to generate UID

#define BATTERY_FULL            3000   // in mV, use max measured voltage
#define BATTERY_EMPTY           2500   // in mV, use cut-off voltage

#define RADIO_POWER_PIN         A5
#define RADIO_RX_PIN            4      // Not connected
#define RADIO_TX_PIN            10      // Data pin for sender
#define RADIO_PTT_PIN           6      // Not connected
#define RADIO_REPEAT_MSG        5      // Normally 3 times is enough to ensure data transmission
#define RADIO_BAUD_RATE         2000   // Needs to be adjusted on receiver side, too

#define LED_PIN                 13
#define REED_PIN                2

const boolean DEBUG = false;

#define INT_MIN -32768         // Value for "no sensor value available"
int mp = 0;                    // Message Pointer
const int START_SEQ = 0xFF;    // Start sequence of a message
const uint8_t PROTOCOL_VERSION = 1;

const int V_STATUS = 2;

uint8_t MESSAGE_SEQUENCE_NUMBER = 0;
uint16_t DEVICE_UID = 0;       // is read from EEPROM in setup or generated if not present
                               // Value range is from 10000 to 60000

union {
  int i = 0;
  unsigned char b[2];
} uid, bat, reed;


// from http://provideyourown.com/2012/secret-arduino-voltmeter-measure-battery-voltage/
int readVcc() {
  // Read 1.1V reference against AVcc
  // set the reference to Vcc and the measurement to the internal 1.1V reference
  #if defined(__AVR_ATmega32U4__) || defined(__AVR_ATmega1280__) || defined(__AVR_ATmega2560__)
    ADMUX = _BV(REFS0) | _BV(MUX4) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #elif defined (__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
    ADMUX = _BV(MUX5) | _BV(MUX0);
  #elif defined (__AVR_ATtiny25__) || defined(__AVR_ATtiny45__) || defined(__AVR_ATtiny85__)
    ADMUX = _BV(MUX3) | _BV(MUX2);
  #else
    ADMUX = _BV(REFS0) | _BV(MUX3) | _BV(MUX2) | _BV(MUX1);
  #endif  
 
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Start conversion
  while (bit_is_set(ADCSRA,ADSC)); // measuring
 
  uint8_t low  = ADCL; // must read ADCL first - it then locks ADCH  
  uint8_t high = ADCH; // unlocks both
 
  long result = (high<<8) | low;
 
  result = 1125300L / result; // Calculate Vcc (in mV); 1125300 = 1.1*1023*1000
  return int(result); // Vcc in millivolts
}

void wake () {
  sleep_disable();
  detachInterrupt (0);
}

void setup () {
  if(DEBUG) Serial.begin(57600);

  if(DEBUG) {
    Serial.print("PROTOCOL_VERSION: ");
    Serial.println(PROTOCOL_VERSION);
  }
  
  randomSeed(analogRead(RANDOM_PIN));

  // Clear EEPROM DEVICE_UID
  //EEPROM.write(0, 0);
  //EEPROM.write(1, 0);
  
  uid.b[0] = EEPROM.read(0);
  uid.b[1] = EEPROM.read(1);
  DEVICE_UID = uint16_t(uid.i);
  if(DEVICE_UID == 0) {
    if(DEBUG) Serial.println("Generating new DEVICE_UID because nothing found in EEPROM");
    DEVICE_UID = random(10000, 60000); 
    uid.i = DEVICE_UID;
    EEPROM.write(0, uid.b[0]);
    EEPROM.write(1, uid.b[1]);
  }
  if(DEBUG) {
    Serial.print("DEVICE_UID:       ");
    Serial.println(DEVICE_UID);
  }

  pinMode(LED_PIN, OUTPUT);
  pinMode(RADIO_POWER_PIN, OUTPUT);
  digitalWrite(RADIO_POWER_PIN, HIGH);
  
  vw_set_tx_pin(RADIO_TX_PIN);
  vw_set_rx_pin(RADIO_RX_PIN);   // Not connected
  vw_set_ptt_pin(RADIO_PTT_PIN); // Not connected
  vw_setup(RADIO_BAUD_RATE);
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
  reed.i = state;
  if(DEBUG) Serial.println(state);

  mp = 7; // Set message pointer after battery readings
  // INITIALIZE SENSOR VALUES
  bat.i    = INT_MIN;

  MESSAGE_SEQUENCE_NUMBER+=1; // increase sequence number to avoid duplicate messages
  if(DEBUG) {
    Serial.print("Message Sequence Number: ");
    Serial.print(MESSAGE_SEQUENCE_NUMBER);
  }

  //               Protocoll
  //               |Message sequence number
  //               ||Device UID
  //               ||| Battery information
  //               ||| | Reed switch status
  byte msg[10] = "S    BBRR";
  //              012345678

  // copy static values to message
  memcpy(msg+0,  &START_SEQ, 1);
  memcpy(msg+1,  &PROTOCOL_VERSION, 1);
  memcpy(msg+2,  &MESSAGE_SEQUENCE_NUMBER, 1);
  memcpy(msg+3,  &DEVICE_UID, 2);
  
  /*************************
  *** READ BATTERY START ***
  **************************/
  int batMv = readVcc();
  bat.i = (float(batMv-BATTERY_EMPTY) / float(BATTERY_FULL-BATTERY_EMPTY)) * 10000.0f;
  if(bat.i > 10000)
    bat.i = 10000;
  if(DEBUG) {
    Serial.print("   Battery: ");
    Serial.print(batMv);
    Serial.print(" mV (");
    Serial.print(float(bat.i/100.0f));
    Serial.println(" %)");
  }
  memcpy(msg+5, bat.b, 2);
  /*************************
  ***  READ BATTERY END  ***
  **************************/
  
  memcpy(msg+mp+0, &V_STATUS, 1);
  memcpy(msg+mp+1, reed.b, 2);
  mp += 3;
  
  digitalWrite(LED_PIN, HIGH);
  delay(5);
  digitalWrite(LED_PIN, LOW);
  
  // Send data
  for(int i=0; i<RADIO_REPEAT_MSG; i++) {
    vw_send((uint8_t *)msg, mp);
    vw_wait_tx();
    delay(100);
  }
  
  int test = digitalRead(2);
  if(state == test) {
    if(state == 1) {
      up();
    } else {
      down();
    }
  }
}