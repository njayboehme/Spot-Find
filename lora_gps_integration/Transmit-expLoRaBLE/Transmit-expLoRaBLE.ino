// 19 jan 2022
// tx-exp

#include <Adafruit_GPS.h>
#include <RadioLib.h> 

/* 

   RadioLib SX126x Transmit Example 
   Transmits packets using SX1262 LoRa radio module. 
   Each packet contains up to 256 bytes of data, in the form of:
    - arbitrary binary data (byte array)  
   For default module settings, see the wiki page
    https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem 
    For full API reference, see the GitHub Pages 
     https://jgromes.github.io/RadioLib/ 

     Download the example code here for a transmitter/receiver for LoRa to LoRa comms
     https://learn.sparkfun.com/tutorials/sparkfun-explorable-hookup-guide/peer-to-peer-example
*/ 


// SX1262 has the following connections: 
// NSS pin:   D36 
// DIO1 pin:  D40 
// NRESET pin:  D44 
// BUSY pin:  D39 
SX1262 radio = new Module(D36, D40, D44, D39, SPI1);

// Hardware Serial Port for GPS
#define GPSSerial Serial1 

// Connect to the GPS on the hardware port 
Adafruit_GPS GPS(&GPSSerial); 
// Set GPSECHO to 'false' to turn off echoing the GPS data to the Serial console 
// Set to 'true' if you want to debug and listen to the raw GPS sentences 
#define GPSECHO false

uint32_t timer = millis(); 

void transmit_MSG(int32_t lat_fixed, int32_t lng_fixed){

  // lat_fixed = 0xFFAAFFAA;
  // lng_fixed = 0xCCBBCCBB;
  Serial.print(F("[SX1262] Transmitting packet ... ")); 
  Serial.print("GPS fix: "); Serial.print((int)GPS.fix);
  Serial.print (" quality: "); Serial.println((int)GPS.fixquality);
  uint8_t lat_1MSB = (lat_fixed & 0xFF000000) >> 24;
  uint8_t lat_2MSB = (lat_fixed & 0x00FF0000) >> 16;
  uint8_t lat_3MSB = (lat_fixed & 0x0000FF00) >> 8;
  uint8_t lat_4MSB = (lat_fixed & 0x000000FF) >> 0;
  uint8_t lng_1MSB = (lng_fixed & 0xFF000000) >> 24;
  uint8_t lng_2MSB = (lng_fixed & 0x00FF0000) >> 16;
  uint8_t lng_3MSB = (lng_fixed & 0x0000FF00) >> 8;
  uint8_t lng_4MSB = (lng_fixed & 0x000000FF) >> 0;
    
  byte byteArr[] = {lat_1MSB, lat_2MSB, lat_3MSB, lat_4MSB, lng_1MSB, lng_2MSB, lng_3MSB, lng_4MSB}; 
  int state = radio.transmit(byteArr, 8);
  
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

void setup() { 
  //while(!Serial);
  Serial.begin(9600); 
  Serial.println("Adafruit GPS and Sparkfun LoRa integration test!");

 /* GPS Setup
  */
  GPS.begin(9600);
  // uncomment this line to turn on RMC (recommended minimum) and GGA (fix data) including altitude 
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA); 
  // 9600 NMEA is the default baud rate for Adafruit MTK GPS's- some use 4800 

  // uncomment this line to turn on only the "minimum recommended" data 
  // GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCONLY); 
  // For parsing data, we don't suggest using anything but either RMC only or RMC+GGA since 
  // the parser doesn't care about other sentences at this time 
  // Set the update rate 

  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 Hz update rate for GPS location 

  // For the parsing code to work nicely and have time to sort thru the data, and 

  // print it out we don't suggest using anything higher than 1 Hz 

 

  // Request updates on antenna status, comment out to keep quiet 
  GPS.sendCommand(PGCMD_ANTENNA); 
  delay(1000);
  GPSSerial.println(PMTK_Q_RELEASE); 
 /* 
 *  End GPS Setup
 */
   

  // initialize SX1262 with default settings 
  Serial.print(F("[SX1262] Initializing ... "));
  // carrier freq, bandwidth, spreading factor, coding rate denominator, syncWord, power, preambleLength, TCXP reference voltage, useRegulatorLDO
  // OG: int state = radio.begin(903.9, 250.0, 9, 5, 0x34, 20, 10, 0, false);
  int state = radio.begin(915.0, 250.0, 9, 5, 0x34, 20, 10, 0, false);
  if (state == RADIOLIB_ERR_NONE) { 
    Serial.println(F("init success!")); 
  } else {
    Serial.print(F("init failed, code "));
    Serial.println(state); 
    while (true); 
  } 

} 

 

void loop() { 

  // Read data from GPS
  char c = GPS.read(); 
 
  if (GPS.newNMEAreceived()) {
    // a tricky thing here is if we print the NMEA sentence, or data
    // we end up not listening and catching other sentences!
    // so be very wary if using OUTPUT_ALLDATA and trying to print out data
    Serial.print(GPS.lastNMEA()); // this also sets the newNMEAreceived() flag to false
    if (!GPS.parse(GPS.lastNMEA())) // this also sets the newNMEAreceived() flag to false
      return; // we can fail to parse a sentence in which case we should just wait for another
  }

  // wait one second between LoRa transmissions
  if(millis() - timer >= 1000){
    timer = millis();
    int32_t lat_fixed = GPS.latitude_fixed;
    int32_t lng_fixed = GPS.longitude_fixed;
    transmit_MSG(lat_fixed, lng_fixed);

    //int state = radio.transmit("Hello, world!");

    // if (state == ERR_NONE) { 
    //   Serial.println("Tx success.");
    // }
    
    Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
    Serial.print("Latitude: "); Serial.println(lat_fixed);
    Serial.print("Longitude: "); Serial.println(lng_fixed);
  }

}  
