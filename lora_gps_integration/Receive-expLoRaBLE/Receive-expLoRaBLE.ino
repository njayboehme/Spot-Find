// 27 jan 2022
// rx-exp

#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
//SX1262 radio = new Module(10, 2, 3, 9);
SX1262 radio = new Module(D36, D40, D44, D39);

void setup() {
  Serial.begin(9600);

  // initialize SX1262 with default settings
  int state = radio.begin(903.9, 250.0, 12, 5, 0x34, 20, 10, 0, false);

  if (state == ERR_NONE) {
    //Serial.println(F("success!"));
  } else {
    //Serial.print(F("failed, code "));
    //Serial.println(state);
    while (true);
  }
}

union {
    float rssi_rec;
    uint8_t rssi_rec_arr[4];
} rssi_union;

void loop() {
  //Serial.print(F("[SX1262] Waiting for incoming transmission ... "));
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
   uint8_t packet_counter_rec = byteArr[12];

  if (state == ERR_NONE) {
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
    Serial.print(packet_counter_rec);
    Serial.print("\r\n");

  } else if (state == ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet

  } else if (state == ERR_CRC_MISMATCH) {
    // packet was received, but is malformed

  } else {
    // some other error occurred
//     Serial.print(F("failed, code "));
//     Serial.println(state);
  }
}
