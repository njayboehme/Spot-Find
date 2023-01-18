// 27 jan 2022
// rx-exp

#include <RadioLib.h>

// See this website for the receiver example code
// https://learn.sparkfun.com/tutorials/sparkfun-explorable-hookup-guide/peer-to-peer-example
// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
//SX1262 radio = new Module(10, 2, 3, 9);
SX1262 radio = new Module(D36, D40, D44, D39, SPI1);

void setup() {
  Serial.begin(9600);

  // initialize SX1262 with default settings
  // OG Settings
  // int state = radio.begin(903.9, 250.0, 12, 5, 0x34, 20, 10, 0, false);
  int state = radio.begin(915.0, 250.0, 9, 5, 0x34, 20, 10, 0, false);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("init success!"));
  } else {
    Serial.print(F("init failed, code "));
    Serial.println(state);
    while (true);
  }
}

union {
    float rssi_rec;
    uint8_t rssi_rec_arr[4];
} rssi_union;

void loop() {
  // Serial.print(F("[SX1262] Waiting for incoming transmission ... "));
  Serial.print("."); 

  // Receive data as a byte array.
  byte byteArr[13];
  int state = radio.receive(byteArr, 13);
  int32_t lat_fixed_recovered = (byteArr[0] << 24) | (byteArr[1] << 16) | (byteArr[2] << 8) | byteArr[3];
  int32_t lng_fixed_recovered = (byteArr[4] << 24) | (byteArr[5] << 16) | (byteArr[6] << 8) | byteArr[7];
  for(uint8_t i = 0; i < 4; i++){
    rssi_union.rssi_rec_arr[i] = byteArr[i + 8];
  }
  //uint32_t packet_counter_rec = (byteArr[12] << 24) | (byteArr[13] << 16) | (byteArr[14] << 8) | byteArr[15];
  // uint32_t packet_counter_rec = (byteArr[12] << 24) | (byteArr[13] << 16) | (byteArr[14] << 8) | byteArr[15];
  uint8_t packet_counter_rec = byteArr[12];

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
    //Serial.println(F("success!"));
    Serial.print("*");
    // print the data of the packet
    Serial.print(lat_fixed_recovered); 
    Serial.print(",");              
    Serial.print(lng_fixed_recovered);  
    Serial.print(",");
    Serial.print(rssi_union.rssi_rec);  
    Serial.print(",");

    // print the LoRa RSSI
    // of the last received packet
    Serial.print(radio.getRSSI()); // units: dBm
    Serial.print(",");

    // print the LoRa SNR 
    // of the last received packet
    Serial.print(radio.getSNR()); // units: dB
    Serial.print(",");
    // Serial.print(packet_counter_rec);
    Serial.print("\r\n");

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout!"));
  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error!"));
  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);
  }
}
