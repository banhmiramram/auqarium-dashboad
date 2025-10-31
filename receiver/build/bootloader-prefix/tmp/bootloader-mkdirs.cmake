# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/hiep/esp-idf/components/bootloader/subproject"
  "/home/hiep/Hiep/tft/build/bootloader"
  "/home/hiep/Hiep/tft/build/bootloader-prefix"
  "/home/hiep/Hiep/tft/build/bootloader-prefix/tmp"
  "/home/hiep/Hiep/tft/build/bootloader-prefix/src/bootloader-stamp"
  "/home/hiep/Hiep/tft/build/bootloader-prefix/src"
  "/home/hiep/Hiep/tft/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/hiep/Hiep/tft/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/hiep/Hiep/tft/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
