
#include <Wire.h>

union {
    float double_var;
    uint8_t bytes_array[4];
} phase_union;

TwoWire myWire(D16 , D17);
const int DATA_SIZE = 255;
byte data_buffer[DATA_SIZE];
const int num_phases_aug = 65;
float phases_1[num_phases_aug];
float phases_2[num_phases_aug];
const int data_split = 32;

void setup()
{
  myWire.begin();        // join i2c bus (address optional for master)
  myWire.setClock(100000);
  pinMode(8, OUTPUT);
  digitalWrite(8, LOW);
  Serial.begin(9600);  // start serial for output
}

void loop()
{
  
  digitalWrite(8, HIGH);
  delay(1000);
  Serial.print("---------------FIRST ESP32 (below)----------------------");
  Serial.println();
  int bytes_in = myWire.requestFrom(0x28, 4);    // request 6 bytes from slave device #2
//  Serial.print("Bytes received: ");
//  Serial.print(bytes_in);
//  Serial.print("\n");
  while(myWire.available())    // slave may send less than requested
  {
//    Serial.print("Bytes available: ");
//    Serial.print(myWire.available());
//    Serial.print("\n");

    myWire.readBytes(data_buffer, myWire.available());

//    Serial.print("Reading bytes: ");
//    Serial.print(myWire.available());
//    Serial.print("\n");

    //phase_union phase;
    int idx = 0;
    for (size_t i = 0; i < 4; i++) {
      phase_union.bytes_array[i] = data_buffer[idx];
//      Serial.print(phase_union.bytes_array[i], HEX);
//      Serial.print(" ");
      idx++;
    }
    double rssi_1 = phase_union.double_var;
    Serial.print(rssi_1);
    Serial.println();
  }
//  Serial.println();
  
//Get data from second ESP32
  Serial.print("---------------SECOND ESP32 (below)----------------------");
  Serial.println();
  bytes_in = myWire.requestFrom(0x29, 4);
//  Serial.print("Bytes received: ");
//  Serial.print(bytes_in);
//  Serial.print("\n");

  while(myWire.available())    // slave may send less than requested
  {
//    Serial.print("Bytes available: ");
//    Serial.print(myWire.available());
//    Serial.print("\n");

    myWire.readBytes(data_buffer, myWire.available());

//    Serial.print("Reading bytes: ");
//    Serial.print(myWire.available());
//    Serial.print("\n");

  //phase_union phase;
    int idx = 0;
    for (size_t i = 0; i < 4; i++) {
      phase_union.bytes_array[i] = data_buffer[idx];
//      Serial.print(phase_union.bytes_array[i], HEX);
//      Serial.print(" ");
      idx++;
    }
    double rssi_2 = phase_union.double_var;
    Serial.print(rssi_2);
    Serial.println();
  
//    Serial.println();
  }

//  Serial.print("\nWaiting....\n");
  digitalWrite(8, LOW);
  delay(1000);
}
