        This program  reads the angles and heading from the accelerometer, gyroscope
        and compass on a BerryIMU connected to an Arduino.

       The BerryIMUv1, BerryIMUv2 and BerryIMUv3 have their supporting .h files
       However, the current code is meant for the v3 alone, as the others two
       require certain changes in the arithmatic of the mag and acc values 

       Code is templated off the github directory from ozzmaker
       https://ozzmaker.com/berryimu/
