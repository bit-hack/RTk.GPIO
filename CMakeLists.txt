add_library(gpio
    gpio.h
    WiringPi.h
    WiringPiSPI.h
    gpio.cpp)

target_include_directories(gpio PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
