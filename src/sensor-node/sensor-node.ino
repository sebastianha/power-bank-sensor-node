#include <stdint.h>
#include <EEPROM.h>
#include <VirtualWire.h>
#include <LowPower.h>  // 2,45 mA

/*********************************
***** HARDWARE CONFIGURATION *****
**********************************/

#define RANDOM_PIN              1      // Needs to be an unconnected ANALOG pin to generate UID

#define SLEEP_TIME_8S           7      // Default: 37 for ~5 minutes

#define BATTERY_FULL            4200   // in mV, use max measured voltage
#define BATTERY_EMPTY           2800   // in mV, use cut-off voltage

#define RADIO_RX_PIN            2      // Not connected
#define RADIO_TX_PIN            A3     // Data pin for sender
#define RADIO_PTT_PIN           3      // Not connected
#define RADIO_REPEAT_MSG        3      // Normally 3 times is enough to ensure data transmission
#define RADIO_BAUD_RATE         2000   // Needs to be adjusted on receiver side, too

#define DHT22_TEMP                     // Enable if DHT22 is connected and temperature should be read
#define DHT22_TEMP_MULTIPLIER   1      // Multiplier
#define DHT22_TEMP_OFFSET       0      // Offset in °C
#define DHT22_HUM                      // Enable if DHT22 is connected and humidity should be read
#define DHT22_HUM_MULTIPLIER    1      // Multiplier
#define DHT22_HUM_OFFSET        0      // Offset in %
#define DHT22_POWER_PIN         13     // Powers the sensor
#define DHT22_PIN               A0     // Data pin for sensor

#define BARO_TEMP                      // Enable if BMP180 is connected and temperature should be read
#define BARO_TEMP_MULTIPLIER    1      // Multiplier
#define BARO_TEMP_OFFSET        0      // Offset in °C
#define BARO_PRESS                     // Enable if BMP180 is connected and pressure should be read
#define BARO_PRESS_MULTIPLIER   1      // Multiplier
#define BARO_PRESS_OFFSET       0      // Offset in mbar
#define BARO_POWER_PIN          5      // Powers the sensor

#define DS18B20                        // Enable if DS18B20 is connected
#define DS18B20_MULTIPLIER      1      // Multiplier
#define DS18B20_OFFSET          0      // Offset in °C
#define DS18B20_POWER_PIN       4      // Powers the sensor
#define DS18B20_PIN             6      // Data pin for sensor

/****************************
***** DO NOT EDIT BELOW *****
*****************************/

const boolean DEBUG = false;   // Enable debug mode?

#define INT_MIN -32768         // Value for "no sensor value available"
int mp = 0;                    // Message Pointer

const int START_SEQ = 0xFF;    // Start sequence of a message
const uint8_t PROTOCOL_VERSION = 1;

// According to mysensors definition
const int V_TEMP     = 0;
const int V_HUM      = 1;
const int V_PRESSURE = 4;

uint8_t MESSAGE_SEQUENCE_NUMBER = 0;
uint16_t DEVICE_UID = 0;       // is read from EEPROM in setup or generated if not present
                               // Value range is from 10000 to 60000

#if defined(DHT22_TEMP) || defined(DHT22_HUM)
  #include <dht.h>
  dht DHT;
#endif

#if defined(BARO_TEMP) || defined(BARO_PRESS)
  #include <Wire.h>
  #include <SFE_BMP180.h>
  SFE_BMP180 pressure;
#endif

#ifdef DS18B20
  #include <OneWire.h>
  OneWire ds(DS18B20_PIN);
#endif

union {
  int i = 0;
  unsigned char b[2];
} uid, bat, temp, hum, baro_t, ds_t1;

union {
  float f = 0;
  unsigned char b[4];
} baro_p;

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

void measure(void) {
  mp = 7; // Set message pointer after battery readings
  // INITIALIZE SENSOR VALUES
  bat.i    = INT_MIN;
  temp.i   = INT_MIN;
  hum.i    = INT_MIN;
  baro_t.i = INT_MIN;
  ds_t1.i  = INT_MIN;
  baro_p.f = INT_MIN;

  MESSAGE_SEQUENCE_NUMBER+=1; // increase sequence number to avoid duplicate messages
  if(DEBUG) {
    Serial.print("Message Sequence Number: ");
    Serial.print(MESSAGE_SEQUENCE_NUMBER);
  }

  //               Protocoll
  //               |Message sequence number
  //               ||Device UID
  //               ||| Battery information
  //               ||| | DHT22 Temperature
  //               ||| | |  DHT22 Humidity
  //               ||| | |  |  BMP180 Temperature
  //               ||| | |  |  |  BMP180 Pressure
  //               ||| | |  |  |  |    DS18B20 Temperatures
  //               ||| | |  |  |  |    |
  byte msg[28] = "S    BBTTTHHHTTTPPPPPTTTTTT";
  //              012345678901234567890123456

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
  
  // Power up sensors and wait some time to "charge"
  #if defined(DHT22_TEMP) || defined(DHT22_HUM)
    digitalWrite(DHT22_POWER_PIN, HIGH);
    digitalWrite(DHT22_PIN, HIGH);
  #endif
  #ifdef DS18B20
    digitalWrite(DS18B20_POWER_PIN, HIGH);
  #endif
  
  delay(700);
  
  /*************************
  ***  READ DHT22 START  ***
  **************************/
  #if defined(DHT22_TEMP) || defined(DHT22_HUM)
    if(DHT.read22(DHT22_PIN) == DHTLIB_OK) {
      temp.i = int((DHT.temperature * DHT22_TEMP_MULTIPLIER + DHT22_TEMP_OFFSET) * 100);
      hum.i  = int((DHT.humidity    * DHT22_HUM_MULTIPLIER  + DHT22_HUM_OFFSET ) * 100);

      if(DEBUG) {
        Serial.print("Temp1: ");
        Serial.print(float(temp.i/100.0f));
        Serial.print(" C   Humid: ");
        Serial.print(float(hum.i/100.0f));
        Serial.print(" %");
      }
    }

    #ifdef DHT22_TEMP
      memcpy(msg+mp+0, &V_TEMP, 1);
      memcpy(msg+mp+1, temp.b, 2);
      mp += 3;
    #endif
    #ifdef DHT22_HUM
      memcpy(msg+mp+0, &V_HUM, 1);
      memcpy(msg+mp+1, hum.b, 2);
      mp += 3;
    #endif
  #endif
  /*************************
  ***   READ DHT22 END   ***
  **************************/

  /*************************
  ***   READ BMP START   ***
  **************************/
  #if defined(BARO_TEMP) || defined(BARO_PRESS)
    char status;
    double T,P;

    status = pressure.startTemperature();
    if (status != 0) {
      delay(status);
      status = pressure.getTemperature(T);
      if (status != 0) {
        status = pressure.startPressure(3);
        if (status != 0) {
          delay(status);
          status = pressure.getPressure(P,T);
          if (status != 0) {
            baro_t.i = int(( T * BARO_TEMP_MULTIPLIER  + BARO_TEMP_OFFSET ) * 100);
            baro_p.f = float(P * BARO_PRESS_MULTIPLIER + BARO_PRESS_OFFSET);
            if(DEBUG) {
              Serial.print("   Temp2: ");
              Serial.print(float(baro_t.i/100.0f));
              Serial.print(" C   Press: ");
              Serial.print(float(baro_p.f));
              Serial.print(" mbar");
            }
          }
        }
      }
    }

    #ifdef BARO_TEMP
      memcpy(msg+mp+0, &V_TEMP, 1);
      memcpy(msg+mp+1, baro_t.b, 2);
      mp += 3;
    #endif
    #ifdef BARO_PRESS
      memcpy(msg+mp+0, &V_PRESSURE, 1);
      memcpy(msg+mp+1, baro_p.b, 4);
      mp += 5;
    #endif
  #endif
  /*************************
  ***    READ BMP END    ***
  **************************/
  
  /*************************
  *** READ DS18B20 START ***
  **************************/
  #ifdef DS18B20
    byte addr[8];
    byte data[12];
    while(ds.search(addr)) {
      if(!OneWire::crc8(addr, 7) != addr[7]) {
        ds_t1.i  = INT_MIN;
        ds.reset();
        ds.select(addr);
        ds.write(0x44, 1);
        // Does this work correctly?
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF);
        ds.reset();
        ds.select(addr);
        ds.write(0xBE);
        for (int i = 0; i < 9; i++) {
          data[i] = ds.read();
        }
        int16_t raw = (data[1] << 8) | data[0];
        byte cfg = (data[4] & 0x60);
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        ds_t1.i = int(((raw / 16.0) * DS18B20_MULTIPLIER + DS18B20_OFFSET) * 100);
        if(DEBUG) {
          Serial.print("   Temp3: ");
          Serial.print(float(ds_t1.i/100.0f));
          Serial.print(" C");
        }

        memcpy(msg+mp+0, &V_TEMP, 1);
        memcpy(msg+mp+1, ds_t1.b, 2);
        mp += 3;
      }
    }
    ds.reset_search();
  #endif
  /*************************
  ***  READ DS18B20 END  ***
  **************************/

  if(DEBUG) Serial.println();

  // Power down sensors
  #if defined(DHT22_TEMP) || defined(DHT22_HUM)
    digitalWrite(DHT22_POWER_PIN, LOW);
    digitalWrite(DHT22_PIN, LOW);
  #endif
  #ifdef DS18B20
    digitalWrite(DS18B20_POWER_PIN, LOW);
  #endif

  // Send data
  for(int i=0; i<RADIO_REPEAT_MSG; i++) {
    vw_send((uint8_t *)msg, mp);
    vw_wait_tx();
    delay(100);
  }
}

void setup(void) {
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

  vw_set_tx_pin(RADIO_TX_PIN);
  vw_set_rx_pin(RADIO_RX_PIN);   // Not connected
  vw_set_ptt_pin(RADIO_PTT_PIN); // Not connected
  vw_setup(RADIO_BAUD_RATE);

  #if defined(DHT22_TEMP) || defined(DHT22_HUM)
    pinMode(DHT22_POWER_PIN, OUTPUT);
    pinMode(DHT22_PIN, OUTPUT);
  #endif

  #if defined(BARO_TEMP) || defined(BARO_PRESS)
    pinMode(BARO_POWER_PIN, OUTPUT);
    digitalWrite(BARO_POWER_PIN, HIGH);
    delay(500);
    if (pressure.begin())
      if(DEBUG) Serial.println("BMP180 init success");
    else
      if(DEBUG) Serial.println("BMP180 init fail");
  #endif

  #ifdef DS18B20
    pinMode(DS18B20_POWER_PIN, OUTPUT);
  #endif

  if(DEBUG) Serial.println("\n");
}

void loop(void) {
  int t;
  if(DEBUG) t = millis();
  measure();
  if(DEBUG) {
    Serial.print("Measuring data took ");
    Serial.print(((millis()-t)));
    Serial.println(" ms");
    Serial.println();
  }
  
  // Every ~5 minutes (37*8s)
  if(DEBUG)
    delay(3000);
  else
    for(int i=0; i<SLEEP_TIME_8S; i++)
      LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
}

