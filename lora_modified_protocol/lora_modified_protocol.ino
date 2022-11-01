/*
   RadioLib SX126x Receive Example

   This example listens for LoRa transmissions using SX126x Lora modules.
   To successfully receive data, the following settings have to be the same
   on both transmitter and receiver:
    - carrier frequency
    - bandwidth
    - spreading factor
    - coding rate
    - sync word
    - preamble length

   Other modules from SX126x family can also be used.

   For default module settings, see the wiki page
   https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

   For full API reference, see the GitHub Pages
   https://jgromes.github.io/RadioLib/
*/

// include the library
#include <RadioLib.h>

// SX1262 has the following connections:
// NSS pin:   10
// DIO1 pin:  2
// NRST pin:  3
// BUSY pin:  9
SX1262 radio = new Module(10, 2, 3, 9);
//SX1262 radio = new Module(D36, D40, D44, D39);

// or using RadioShield
// https://github.com/jgromes/RadioShield
//SX1262 radio = RadioShield.ModuleA;

void setup() {
  Serial.begin(9600);

  // initialize SX1262 with default settings
  Serial.print(F("[SX1262] Initializing ... "));
  //int state = radio.begin();
  //int state = radio.begin(915.0, 250.0, 7, 5, 0x34, 20, 10, 0, false);
  int state = radio.begin(903.9, 250.0, 9, 5, 0x34, 20, 10, 0, false);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("success!"));
  } else {
    Serial.print(F("failed, code "));
    Serial.println(state);
    while (true);
  }
}

void loop() {
  Serial.print(F("[SX1262] Waiting for incoming transmission ... "));

  // you can receive data as an Arduino String
  // NOTE: receive() is a blocking method!
  //       See example ReceiveInterrupt for details
  //       on non-blocking reception method.
  String str;
  int state = radio.receive(str);

  // you can also receive data as byte array
/*
  byte byteArr[8];
  int state = radio.receive(byteArr, 8);
*/

  if (state == RADIOLIB_ERR_NONE) {
    // packet was successfully received
//    Serial.println(F("success!"));
//
//    // print the data of the packet
//    Serial.print(F("[SX1262] Data:\t\t"));
//    Serial.println(str);
//    for (int i = 0; i < 8; i++)
//    {
//      Serial.print(byteArr[i]);
//    }
//    Serial.println();

    // print the RSSI (Received Signal Strength Indicator)
    // of the last received packet
//    Serial.print(F("[SX1262] RSSI:\t\t"));
    Serial.print("RSSI: ");
    Serial.print(radio.getRSSI());
    Serial.print(F(" dBm "));

//    // print the SNR (Signal-to-Noise Ratio)
//    // of the last received packet
//    Serial.print(F("[SX1262] SNR:\t\t"));
    Serial.print(radio.getSNR());
    Serial.print(F(" dB"));
    Serial.print(F("\r\n"));

  } else if (state == RADIOLIB_ERR_RX_TIMEOUT) {
    // timeout occurred while waiting for a packet
    Serial.println(F("timeout"));

  } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
    // packet was received, but is malformed
    Serial.println(F("CRC error"));

  } else {
    // some other error occurred
    Serial.print(F("failed, code "));
    Serial.println(state);

  }
  delay(0.5);
}
