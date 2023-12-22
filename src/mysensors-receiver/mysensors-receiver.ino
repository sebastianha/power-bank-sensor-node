#include <stdint.h>
#include <VirtualWire.h>

const int rx_pin   = 11;
const int tx_pin   = 5;      // Not connected
const int ptt_pin  = 6;      // Not connected
const int led_pin  = 13;
const int baudrate = 2000;   // Needs to be adjusted on sender side, too

#define INT_MIN    -32768    // Value for "no sensor value available"

// According to mysensors definition
const int V_TEMP     = 0;
const int V_HUM      = 1;
const int V_STATUS   = 2;
const int V_PRESSURE = 4;
const int V_TRIPPED  = 16;
const int V_LEVEL    = 37;

const int S_DOOR     = 0;
const int S_TEMP     = 6;
const int S_HUM      = 7;
const int S_BARO     = 8;
const int S_LIGHT_LEVEL = 16;

const boolean DEBUG = false; // Enable debug mode?

union {
  unsigned char b[2];
  int i = 0;
} b2i;

union {
  unsigned char b[2];
  unsigned int u = 0;
} b2u;

union {
  unsigned char b[4];
  float f = 0;
} b2f;

uint16_t UID = -1;
int sensor_id = 0;
uint8_t MESSAGE_SEQUENCE_NUMBER = 0;

const uint8_t max_known_uids = 32;
uint16_t known_uids[max_known_uids];
bool uid_is_known(uint16_t uid) {
  for(int i=0; i<max_known_uids; i++) {
    if(DEBUG) {
      Serial.print(uid);
      Serial.print(" ");
      Serial.println(known_uids[i]);
    }

    if(known_uids[i] == uid) {
      if(DEBUG) {
        Serial.print("ID KNOWN: ");
        Serial.println(uid);
      }
      return true;
    }
    if(known_uids[i] == 65535) {
      if(DEBUG) {
        Serial.print("ID UNKNOWN: ");
        Serial.println(uid);
      }
      known_uids[i] = uid;
      return false;
    }
  }
  return false;
}

void print_mysensor_presentation() {
  Serial.print(UID/256);
  Serial.println(";255;0;0;0;2.0");

  Serial.print(UID/256);
  Serial.print(";255;3;0;11;");
  Serial.println(UID);

  Serial.print(UID/256);
  Serial.println(";255;3;0;12;1.0");
}

void print_mysensor_string(int is_known, int message_type, int sub_type, float payload) {
  int s_id = sensor_id;
  if(sensor_id == 0) {
    s_id = 255;
  } else {
    s_id = s_id -1;

    if(!is_known) {
      Serial.print(UID/256);
      Serial.print(";");
      Serial.print(s_id);
      Serial.print(";0;0;");
      switch(sub_type) {
        case V_TEMP:
          Serial.print(S_TEMP);
          Serial.print(";Temperature ");
          break;
        case V_HUM:
          Serial.print(S_HUM);
          Serial.print(";Humidity ");
          break;
        case V_PRESSURE:
          Serial.print(S_BARO);
          Serial.print(";Pressure ");
          break;
        case V_TRIPPED:
          Serial.print(S_DOOR);
          Serial.print(";Contact ");
          break;
        case V_LEVEL:
          Serial.print(S_LIGHT_LEVEL);
          Serial.print(";Light ");
          break;
        default:
          Serial.print(";Unknown");
      }
      Serial.println(UID);
    }
  }

  Serial.print(UID/256);
  Serial.print(";");
  Serial.print(s_id);
  Serial.print(";");
  Serial.print(message_type);
  Serial.print(";");
  Serial.print(0);
  Serial.print(";");
  Serial.print(sub_type);
  Serial.print(";");
  if(message_type == 1 && (sub_type == V_TEMP || sub_type == V_HUM)) {
    Serial.println(payload);
  } else {
    Serial.println((int)payload);
  }

  sensor_id+=1;
}

void setup()
{
  Serial.begin(115200);

  for(int i=0; i<max_known_uids; i++) {
    known_uids[i] = 65535;
  }

  vw_set_tx_pin(tx_pin);
  vw_set_rx_pin(rx_pin);
  vw_set_ptt_pin(ptt_pin);
  vw_setup(baudrate);

  vw_rx_start();

  pinMode(led_pin, OUTPUT);
  
  pinMode(9, OUTPUT);
  digitalWrite(9, HIGH);
}

void loop()
{
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  sensor_id = 0;

  if (vw_get_message(buf, &buflen)) {
    int i;

    digitalWrite(led_pin, HIGH);

    if(DEBUG) {
      for (i = 0; i < buflen; i++) {
        if((int)buf[i] < 16)
          Serial.print(0);
        Serial.print(buf[i], HEX);
        Serial.print(' ');
      }
      Serial.println();
    }

    if(buf[0] == 0xFF) {
      uint8_t protocol = buf[1];

      uint8_t message_sequence_number = buf[2];

      if(message_sequence_number != MESSAGE_SEQUENCE_NUMBER) {
        MESSAGE_SEQUENCE_NUMBER = message_sequence_number;

        memcpy(b2i.b, buf+3, 2);
        UID = uint16_t(b2i.i);

        bool is_known = uid_is_known(UID);

        memcpy(b2i.b, buf+5, 2);
        float battery_percent = float(b2i.i)/100.0f;

        if(DEBUG) {
          Serial.println("Start found!");
          Serial.print("Protocol Version: ");
          Serial.println(protocol);
          Serial.print("Message Sequence: ");
          Serial.println(message_sequence_number);
          Serial.print("Device UID:       ");
          Serial.println(UID);
          Serial.print("Battery:          ");
          Serial.print(battery_percent, 2);
          Serial.println(" %");
        }

        if(!is_known) {
          print_mysensor_presentation();
        }
        print_mysensor_string(is_known, 3, 0, battery_percent);

        int reader = 7;

        while(reader < buflen) {
          uint8_t SENSOR_TYPE = buf[reader];
          reader+=1;
          if(SENSOR_TYPE == V_TEMP) {
            if(DEBUG) Serial.println("Found temperature value");
            memcpy(b2i.b, buf+reader, 2);
            if(b2i.i > INT_MIN) {
              float temp = float(b2i.i)/100.0f;
              if(DEBUG) {
                Serial.print("Temperature:      ");
                Serial.print(temp, 2);
                Serial.println(" C");
              }
              print_mysensor_string(is_known, 1, V_TEMP, temp);
            } else if(DEBUG) Serial.println("-> temperature value not set!");
            reader+=2;
          } else if(SENSOR_TYPE == V_HUM) {
            if(DEBUG) Serial.println("Found humidity value");
            memcpy(b2i.b, buf+reader, 2);
            if(b2i.i > INT_MIN) {
              float hum = float(b2i.i)/100.0f;
              if(DEBUG) {
                Serial.print("Humidity:         ");
                Serial.print(hum, 2);
                Serial.println(" %");
              }
              print_mysensor_string(is_known, 1, V_HUM, hum);
            } else if(DEBUG) Serial.println("-> humidity value not set!");
            reader+=2;
          } else if(SENSOR_TYPE == V_STATUS) {
            if(DEBUG) Serial.println("Found status value");
            memcpy(b2i.b, buf+reader, 2);
            if(b2i.i > INT_MIN) {
              boolean status = true;
              if(b2i.i != 0) status = false;
              if(DEBUG) {
                Serial.print("Status:           ");
                Serial.println(status);
              }
              print_mysensor_string(is_known, 1, V_TRIPPED, status);
            } else if(DEBUG) Serial.println("-> status value not set!");
            reader+=2;
          } else if(SENSOR_TYPE == V_PRESSURE) {
            if(DEBUG) Serial.println("Found pressure value");
            memcpy(b2f.b, buf+reader, 4);
            if(b2f.f > INT_MIN) {
              float pressure = float(b2f.f);
              if(DEBUG) {
                Serial.print("Pressure:         ");
                Serial.print(pressure, 2);
                Serial.println(" mB");
              }
              print_mysensor_string(is_known, 1, V_PRESSURE, pressure);
            } else if(DEBUG) Serial.println("-> pressure value not set!");
            reader+=4;
          } else if(SENSOR_TYPE == V_LEVEL) {
            if(DEBUG) Serial.println("Found level value");
            memcpy(b2u.b, buf+reader, 2);
            if(b2u.u > INT_MIN) {
              unsigned int level = b2u.u;
              if(DEBUG) {
                Serial.print("Level:            ");
                Serial.print(level);
                Serial.println(" lx");
              }
              print_mysensor_string(is_known, 1, V_LEVEL, level);
            } else if(DEBUG) Serial.println("-> level value not set!");
            reader+=2;
          }
        }
      }
    }

    digitalWrite(led_pin, LOW);
  }
}
