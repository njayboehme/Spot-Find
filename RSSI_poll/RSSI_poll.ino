
#include <Wire.h>
#include <Adafruit_GPS.h>
#include <RadioLib.h> 

// This is for the Transmitter

static uint8_t packet_counter = 0;

union {
    float double_var;
    uint8_t bytes_array[4];
} phase_union;

TwoWire myWire(D16 , D17); // SDA and SCL line on LoRa
const int DATA_SIZE = 255;
byte data_buffer[DATA_SIZE];
const int num_phases_aug = 65;
float phases_1[num_phases_aug];
float phases_2[num_phases_aug];
const int data_split = 32;

// Initialize LoRa radio
SX1262 radio = new Module(D36, D40, D44, D39, SPI1); 
// Hardware Serial Port for GPS
#define GPSSerial Serial1 

// Connect to the GPS on the hardware port 
Adafruit_GPS GPS(&GPSSerial); 
// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console 
// Set to 'true' if you want to debug and listen to the raw GPS sentences 
#define GPSECHO false 

uint32_t timer = millis(); 

void transmit_MSG(int32_t lat_fixed, int32_t lng_fixed) {

  Serial.print(F("[SX1262] Transmitting packet ... ")); 
  Serial.print("GPS fix: "); Serial.print((int)GPS.fix);
  Serial.print (" quality: "); Serial.println((int)GPS.fixquality);
  Serial.print(" RSSI: "); Serial.println(phase_union.double_var);
  uint8_t lat_1MSB = (lat_fixed & 0xFF000000) >> 24;
  uint8_t lat_2MSB = (lat_fixed & 0x00FF0000) >> 16;
  uint8_t lat_3MSB = (lat_fixed & 0x0000FF00) >> 8;
  uint8_t lat_4MSB = (lat_fixed & 0x000000FF) >> 0;
  uint8_t lng_1MSB = (lng_fixed & 0xFF000000) >> 24;
  uint8_t lng_2MSB = (lng_fixed & 0x00FF0000) >> 16;
  uint8_t lng_3MSB = (lng_fixed & 0x0000FF00) >> 8;
  uint8_t lng_4MSB = (lng_fixed & 0x000000FF) >> 0;
      
  byte byteArr[] = {lat_1MSB, lat_2MSB, lat_3MSB, lat_4MSB,
    lng_1MSB, lng_2MSB, lng_3MSB, lng_4MSB,
      phase_union.bytes_array[0],
      phase_union.bytes_array[1],
      phase_union.bytes_array[2],
      phase_union.bytes_array[3]
      // packet_counter This just counts the number of packets received so far (Noah)
    //  (packet_counter & 0xFF000000) >> 24, This was already commented out (Noah)
    //   (packet_counter & 0x00FF0000) >> 16, See above note
    //    (packet_counter & 0x0000FF00) >> 8, See above note
    //     (packet_counter & 0x000000FF) >> 0 See above note
      }; 
  Serial.print("Packet number: ");
  Serial.println(packet_counter++);
  
  int state = radio.transmit(byteArr, 12);
  
  if (state == RADIOLIB_ERR_NONE) { 

    // the packet was successfully transmitted 
    Serial.println(F("success!")); 
    // print measured data rate 
    Serial.print(F("[SX1262] Datarate:\t")); 
    Serial.print(radio.getDataRate()); 
    Serial.println(F(" bps")); 

  } else if (state == RADIOLIB_ERR_PACKET_TOO_LONG) { 

    // the supplied packet was longer than 256 bytes  
    Serial.println(F("too long!")); 

 

  } else if (state == RADIOLIB_ERR_TX_TIMEOUT) { 
    // timeout occured while transmitting packet 
    Serial.println(F("timeout!")); 
  } else { 
    // some other error occurred 
    Serial.print(F("failed, code ")); 
    Serial.println(state); 
  } 
}

void setup()
{
  myWire.begin();        // join i2c bus (address optional for master)
  myWire.setClock(100000);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  Serial.begin(9600);  // start serial for output

  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); 
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate for GPS location 
  GPS.sendCommand(PGCMD_ANTENNA); 
  delay(1000); //Maybe add a delay to see if this works
  GPSSerial.println(PMTK_Q_RELEASE); 
  
  // init LoRa radio with default settings
  int state = radio.begin(915.0, 250.0, 9, 5, 0x34, 20, 10, 0, false);
  // int state = radio.begin(903.9, 250.0, 12, 5, 0x34, 20, 10, 0, false); This was the og
  if (state == RADIOLIB_ERR_NONE) { 
    Serial.println(F("success!")); 
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state); 
    while (true); 
  } 
}

enum RSSI_GPS_int_st_t {
  RSSI_GPS_int_init_st,
  interrupt_write_st,
  GPS_collect_st,
  data_send_st // cancel interrupt signal as Mealy action here
} current_state = RSSI_GPS_int_init_st;

uint32_t lat_fixed = 0;
uint32_t lng_fixed = 0;
double rssi_1 = 0.0;
// uint32_t indext = 0;

void debugStatePrint(){
  static enum RSSI_GPS_int_st_t previousState;
  static bool firstPass = true;

  if(previousState != current_state || firstPass) {
    firstPass = false;
    previousState = current_state;
    switch(current_state){
      case RSSI_GPS_int_init_st: Serial.println("RSSI_GPS_int_init_st"); break;
      case interrupt_write_st: Serial.println("interrupt_write_st"); break;
      case GPS_collect_st: Serial.println("GPS_collect_st"); break;
      case data_send_st: Serial.println("data_send_st"); break;
      default: break;
    }
  }
}

void loop()
{
  // Actions based on current state
  debugStatePrint();
  switch(current_state){
    case RSSI_GPS_int_init_st:
      {
        lat_fixed = 0;
        lng_fixed = 0;
        rssi_1 = 0.0;
        current_state = interrupt_write_st;
        break;
      }
    case interrupt_write_st: // done
      {
        digitalWrite(8, HIGH);
        current_state = GPS_collect_st;
        break; 
      }
    case GPS_collect_st:
      {
        char c = GPS.read(); 
        if(GPS.newNMEAreceived()){
          if (!GPS.parse(GPS.lastNMEA())){ // this also sets the newNMEAreceived() flag to false
            break; // we can fail to parse a sentence in which case we should just wait for another
          }
          else {
            lat_fixed = GPS.latitude_fixed;
            lng_fixed = GPS.longitude_fixed;
            current_state = data_send_st;
          }
        }
        // else { // This is just for testing when inside a building (Noah)
        //   lat_fixed = 0xFFAAFFAA;
        //   lng_fixed = 0xCCBBCCBB;
        //   current_state = data_send_st;
        // }
        break;
      }
    case data_send_st:
    {
     int bytes_in = myWire.requestFrom(0x29, 4);    // request 4 bytes from slave device (esp32)
     while(myWire.available())    // slave may send less than requested
     {
        Serial.print("Bytes available: ");
        Serial.print(myWire.available());

        Serial.print("\n");

        myWire.readBytes(data_buffer, myWire.available());

        Serial.print("Reading bytes: ");
        Serial.print(myWire.available());
        Serial.print("\n");

        //phase_union phase;
        int idx = 0;
        for (size_t i = 0; i < 4; i++) {
          phase_union.bytes_array[i] = data_buffer[idx];
          Serial.print(phase_union.bytes_array[i], HEX);
          Serial.print(" ");
          idx++;
        }
        // phase_union.double_var = 5.0; // This is for testing
        double rssi_1 = phase_union.double_var;
        Serial.print(rssi_1);
        Serial.println();
      }

      transmit_MSG(lat_fixed, lng_fixed);
      digitalWrite(8, LOW);
      current_state = interrupt_write_st;
      break;
    }
    default: break;
  }

  // //actions
  // switch(current_state){
  //   case RSSI_GPS_int_init_st: break;
  //   case interrupt_write_st: break; // no actions necessary for 1-off state
  //   case GPS_collect_st: break;
  //   case data_send_st: break;
  //   default: break;
  // }
}
