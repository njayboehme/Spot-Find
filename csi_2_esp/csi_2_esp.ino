
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
  delay(2000);
  Serial.println("---------------FIRST ESP32 (below)----------------------");
  int bytes_in = myWire.requestFrom(0x28, data_split*4/*65*4*/);    // request 6 bytes from slave device #2
  Serial.print("Bytes received: ");
  Serial.print(bytes_in);
  Serial.print("\n");
  //myWire.requestFrom(0x27, 32);
  //delay(2000);
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
    for (size_t j = 0; j < data_split; j++) {
      for (size_t i = 0; i < 4; i++) {
        phase_union.bytes_array[i] = data_buffer[idx];
        Serial.print(phase_union.bytes_array[i], HEX);
        Serial.print(" ");
        idx++;
      }
      phases_1[j] = phase_union.double_var;
      Serial.print(phases_1[j]);
      Serial.println();
      //Serial.print(j);
      //Serial.print("Phase: ");
      //Serial.print(phases_1[j], 7);         // print the character
      //Serial.println();
    }
    Serial.println();
  }

  bytes_in = myWire.requestFrom(0x28, (num_phases_aug - data_split)*4/*65*4*/);
  Serial.print("Bytes received: ");
  Serial.print(bytes_in);
  Serial.print("\n");

  while(myWire.available())    // slave may send less than requested
  {
    Serial.print("Bytes available: ");
    Serial.print(myWire.available());
    Serial.print("\n");

    myWire.readBytes(data_buffer, (num_phases_aug - data_split)*4);

    Serial.print("Reading bytes: ");
    Serial.print((num_phases_aug - data_split)*4);
    Serial.print("\n");

    //phase_union phase;
    //float phases[20];
    int idx = 0;
    for (size_t j = data_split; j < num_phases_aug; j++) {
      for (size_t i = 0; i < 4; i++) {
        phase_union.bytes_array[i] = data_buffer[idx];
        Serial.print(phase_union.bytes_array[i], HEX);
        Serial.print(" ");
        idx++;
      }
      phases_1[j] = phase_union.double_var;
      Serial.print(phases_1[j]);
      Serial.println();
      //Serial.print(j);
      //Serial.print("Phase: ");
      //Serial.print(phases_1[j], 7);         // print the character
      //Serial.println();
    }
    Serial.println();
  }

  Serial.print("Phases: ");
  for (int i = 0; i < num_phases_aug; i++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.print(phases_1[i], 7);
    Serial.print(" \n");
  }

//  Serial.print("phases_1 = [");
//  for (int i = 0; i < num_phases_aug; i++) {
//    Serial.print(phases_1[i], 7);
//    if (i != (num_phases_aug - 1)) {
//      Serial.print(" ");
//    }
//  }
  Serial.print("]\n");

//Get data from second ESP32
  Serial.println("---------------SECOND ESP32 (below)----------------------");
  bytes_in = myWire.requestFrom(0x29, data_split*4);
  Serial.print("Bytes received: ");
  Serial.print(bytes_in);
  Serial.print("\n");

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
    for (size_t j = 0; j < data_split; j++) {
      for (size_t i = 0; i < 4; i++) {
        phase_union.bytes_array[i] = data_buffer[idx];
        Serial.print(phase_union.bytes_array[i], HEX);
        Serial.print(" ");
        idx++;
      }
      phases_2[j] = phase_union.double_var;
      Serial.print(phases_2[j]);
      Serial.println();
      //Serial.print(j);
      //Serial.print("Phase: ");
      //Serial.print(phases_2[j], 7);         // print the character
      //Serial.println();
    }
    Serial.println();
  }

  bytes_in = myWire.requestFrom(0x29, (num_phases_aug - data_split)*4/*65*4*/);
  Serial.print("Bytes received: ");
  Serial.print(bytes_in);
  Serial.print("\n");

  while(myWire.available())    // slave may send less than requested
  {
    Serial.print("Bytes available: ");
    Serial.print(myWire.available());
    Serial.print("\n");

    myWire.readBytes(data_buffer, (num_phases_aug - data_split)*4);

    Serial.print("Reading bytes: ");
    Serial.print((num_phases_aug - data_split)*4);
    Serial.print("\n");

    //phase_union phase;
    //float phases[20];
    int idx = 0;
    for (size_t j = data_split; j < num_phases_aug; j++) {
      for (size_t i = 0; i < 4; i++) {
        phase_union.bytes_array[i] = data_buffer[idx];
        Serial.print(phase_union.bytes_array[i], HEX);
        Serial.print(" ");
        idx++;
      }
      phases_2[j] = phase_union.double_var;
      Serial.print(phases_2[j]);
      Serial.println();
      //Serial.print(j);
      //Serial.print("Phase: ");
      //Serial.print(phases_2[j], 7);         // print the character
      //Serial.println();
    }
    Serial.println();
  }

  Serial.print("Phases: ");
  for (int i = 0; i < num_phases_aug; i++) {
    Serial.print(i);
    Serial.print(" ");
    Serial.print(phases_2[i], 7);
    Serial.print(" \n");
  }

//  Serial.print("phases_2 = [");
//  for (int i = 0; i < num_phases_aug; i++) {
//    Serial.print(phases_2[i], 7);
//    if (i != (num_phases_aug - 1)) {
//      Serial.print(" ");
//    }
//  }
//  Serial.print("]\n");

  Serial.println("-----------------PHASE DIFFERENCES-------------------");
  float total_diff = 0;
  float phase_count = 0;
  
  for (size_t i = 3; i < num_phases_aug; i++) {
    
    if (phases_1[i] < 0) {
      phases_1[i] = phases_1[i] + (2 * 3.14159);
    }
    if (phases_2[i] < 0) {
      phases_2[i] = phases_2[i] + (2 * 3.14159);
    }
    float phase_diff = phases_1[i] - phases_2[i];
    if (phase_diff < 0) {
      phase_diff = phase_diff + (2 * 3.14159);
    }
    if (phase_diff != 0) {
      total_diff = total_diff + phase_diff;
      phase_count ++;
    }
    Serial.print(i);
    Serial.print(" Phase Diff: ");
    Serial.print(phase_diff);
    Serial.print("\n");
  }
  
  float ave_diff = total_diff / phase_count;
  
  Serial.print("Average Phase Difference: ");
  Serial.print(ave_diff);
  Serial.print("\n");
  
  Serial.print("\nWaiting....\n");
  digitalWrite(8, LOW);
  delay(6000);
}
