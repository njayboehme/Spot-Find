/*      This program  reads the angles and heading from the accelerometer
        and compass on a BerryIMU connected to an Arduino.

*/

#include "IMU.h"

#define DT  0.5          // Loop time
#define MAG_LPF_FACTOR 0.9  //to smooth out the noise some

byte buff[6];
int accRaw[3];
int magRaw[3];

float accXrad = 0.0;
float accYrad = 0.0;
float sinX = 0.0;
float sinY = 0.0;
float cosX = 0.0;
float cosY = 0.0;
float magXcomp = 0.0;
float magYcomp = 0.0;
float normalize = 0.0;
float rangeX = 0;
float rangeY = 0;
float rangeZ = 0;

int magXmax = -32767;
int magYmax = -32767;
int magZmax = -32767;
int magXmin = 32767;
int magYmin = 32767;
int magZmin = 32767;
int offsetX = 0;
int offsetY = 0;
int offsetZ = 0;
int oldXMagRawValue = 0;
int oldYMagRawValue = 0;
int oldZMagRawValue = 0;


char report[80];


unsigned long startTime;

void setup() {
         // join i2c bus (address optional for master)
  Serial.begin(115200);  // start serial for output, make sure the serial monitor/plotter matches this 
  delay(500); // update rate of .5 seconds
  detectIMU();
  
  enableIMU();

}

void loop() {
 startTime = millis();

  //Read the measurements from  sensors
  readACC(buff);
  accRaw[0] = (int)(buff[0] | (buff[1] << 8));   
  accRaw[1] = (int)(buff[2] | (buff[3] << 8));
  accRaw[2] = (int)(buff[4] | (buff[5] << 8));

  readMAG(buff);
  magRaw[0] = (int)(buff[0] | (buff[1] << 8));   
  magRaw[1] = (int)(buff[2] | (buff[3] << 8));
  magRaw[2] = (int)(buff[4] | (buff[5] << 8));

  //Apply low pass filter to reduce noise
  magRaw[0] =  magRaw[0]  * MAG_LPF_FACTOR + oldXMagRawValue*(1 - MAG_LPF_FACTOR);
  magRaw[1] =  magRaw[1]  * MAG_LPF_FACTOR + oldYMagRawValue*(1 - MAG_LPF_FACTOR);
  magRaw[2] =  magRaw[2]  * MAG_LPF_FACTOR + oldZMagRawValue*(1 - MAG_LPF_FACTOR);
  oldXMagRawValue = magRaw[0];
  oldYMagRawValue = magRaw[1];
  oldZMagRawValue = magRaw[2];

  //Calibration, sets the min and max values for the x y and z mag readings
  magXmin = min(magXmin, magRaw[0]);
  magYmin = min(magYmin, magRaw[1]);
  magZmin = min(magZmin, magRaw[2]);
  magXmax = max(magXmax, magRaw[0]);
  magYmax = max(magYmax, magRaw[1]);
  magZmax = max(magZmax, magRaw[2]);

  rangeX = (magXmax - magXmin); // sets ranges
  rangeY = (magYmax - magYmin);
  rangeZ = (magZmax - magZmin);

  offsetX = (magXmin + magXmax) /2 ; // sets offsets
  offsetY = (magYmin + magYmax) /2 ;
  offsetZ = (magZmin + magZmax) /2 ;
  
  //Apply hard iron calibration, sets the offset to center the x y and z ranges around 0
  magRaw[0] -= offsetX; 
  magRaw[1] -= offsetY;
  magRaw[2] -= offsetZ;

  //Apply soft iron calibration, adjusts so x y and z have the same range
  if(rangeX < 0)
            rangeX = rangeX * -1; 
  if(rangeY < 0)
            rangeY = rangeY * -1; 
  if(rangeZ < 0)
            rangeZ = rangeZ * -1; 
  magRaw[1] = magRaw[1] * (rangeX / rangeY);
  magRaw[2] = magRaw[2] * (rangeX / rangeZ);

  //Convert Accelerometer values to radians
  accXrad = (float) (atan2(accRaw[1],accRaw[2]));
  accYrad = (float) (atan2(accRaw[2],accRaw[0]));
  sinX = sin(accXrad); // checking the sin and cos values, will be between -1.00 and 1.00
  sinY = sin(accYrad);
  cosX = cos(accXrad);
  cosY = cos(accYrad);

  //tilt compensation
  magXcomp = (float)(magRaw[0]*cos(accXrad)*sin(accYrad)*sin(accYrad) + magRaw[2]*cos(accXrad)*cos(accYrad));
  magYcomp = (float)(-magRaw[0]*cos(accYrad)*sin(accXrad)+ magRaw[1]*cos(accXrad)*cos(accXrad) + magRaw[2]*sin(accXrad)*sin(accYrad));

  //Compute heading, set north as 0 degrees with east as positive, west as negative
  float headingComp = 180*atan2(magXcomp, -magYcomp)/PI;
  float heading = 180*atan2(magRaw[0], -magRaw[1])/PI;

  //Declination, applied when north is offset to magnetic north from true north
  //float declination = 10.89; // example, magnetic declination in Provo, Utah where testing was done
  //heading += declination;
  //headingComp += declination;
  
  //Convert heading to -180 to 180
          if(heading < -180)
            heading += 360;
          if(heading > 180)
            heading -= 360;
          if(headingComp < -180)
            headingComp += 360;
          if(headingComp > 180)
            headingComp -= 360;
            
  //Serial.print(" # mag0 "); // serial printouts for debugging/checking 
  //Serial.print(magRaw[0]);
  //Serial.print(" # mag1 ");
  //Serial.print(magRaw[1]);
  //Serial.print(" # mag2 ");
  //Serial.print(magRaw[2]);        
  //Serial.print(" # acc0\t ");
  //Serial.print(accRaw[0]);
  //Serial.print(" # acc1\t ");
  //Serial.print(accRaw[1]);
  //Serial.print(" # acc2\t ");
  //Serial.print(accRaw[2]);
  //Serial.print(" # accXrad\t ");
  //Serial.print(accXrad);
  //Serial.print(" # accYrad\t ");
  //Serial.print(accYrad);
  //Serial.print(" # sinX ");
  //Serial.print(sinX);
  //Serial.print(" # cosX ");
  //Serial.print(cosX);
  //Serial.print(" # sinY ");
  //Serial.print(sinY);
  //Serial.print(" # cosY ");
  //Serial.print(cosY);
  //Serial.print(" # magX ");
  //Serial.print(magXcomp);
  //Serial.print(" # magY ");
  //Serial.print(-magYcomp);
  //Serial.print(" # heading\t ");
  //Serial.print(heading); 
  Serial.print(" # headingComp ");
  Serial.print(headingComp); 
  Serial.println(" # Loop Time #\t");

  //Each loop should be at least 20ms.
  while(millis() - startTime < (DT*1000))
        {
            delay(1);
        }
  //Serial.println( millis()- startTime);
 


}
