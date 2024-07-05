# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "D:/DATA/Program/ESP-IDF/components/bootloader/subproject"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/tmp"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/src/bootloader-stamp"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/src"
  "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/src/bootloader-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/src/bootloader-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "D:/Users/nikita/Documents/Crocoone/crocoone_refactoring/ESP_crocoone/build/bootloader-prefix/src/bootloader-stamp${cfgdir}") # cfgdir has leading slash
endif()
