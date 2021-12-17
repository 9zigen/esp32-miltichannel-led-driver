# The following lines of boilerplate have to be in your project's CMakeLists
# in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.5)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(ESP32_CC_LED_DRIVER_RTOS)

set($ENV{IDF_TARGET} "esp32")