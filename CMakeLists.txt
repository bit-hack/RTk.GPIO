cmake_minimum_required(VERSION 3.0)
project(RTkGPIO)

add_library(RTkGPIO
    gpio.h
    WiringPi.h
    WiringPiSPI.h
    gpio.cpp)

target_include_directories(RTkGPIO PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

option(RTkGPIO_examples "Build RTk.GPIO examples" OFF)
if (${RTkGPIO_examples})
  add_subdirectory(examples)
endif()
