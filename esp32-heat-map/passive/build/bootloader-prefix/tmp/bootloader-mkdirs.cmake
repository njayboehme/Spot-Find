# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "C:/Users/njboe/esp/esp-idf/components/bootloader/subproject"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix/tmp"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix/src/bootloader-stamp"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix/src"
  "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "C:/Users/njboe/Desktop/Capstone/Spot-Find/esp32-heat-map/passive/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
