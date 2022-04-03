# RTk.GPIO C++ Interface Library

This is a C++ interface library for controlling the RTk.GPIO board.
The board provides a RaspberryPi like GPIO interface to computers via a USB to Serial chip.

RTk.GPIO boards are available from [https://pi-supply.com](https://pi-supply.com).


### Available APIs

This library provides two c++ APIs for controlling the RTk.GPIO board:
- A `gpio` interface
- A WiringPi interface

Both are very similar and infact the WiringPi interface is implenented directly on top of the `gpio` interface.
The WiringPi interface was added simply to make it easy to port software between platforms.
Not all WiringPi functions are available however due to limitations with the RTk.GPIO board, so it may not work for some applications.


### RTK.GPIO pinout

The GPIO pinout:
```
pin1    3v3     5v
        GP2     5v
        GP3     GND
        GP4     GP14
        GND     GP15
        GP17    GP18
        GP27    GND
        GP22    GP23
        3v3     GP24
        GP10    GND
        GP9     GP25
        GP11    GP8
        GND     GP7
        GP0     GP1
        GP5     GND
        GP6     GP12
        GP13    GND
        GP19    GP16
        GP26    GP20
        GND     GP21    pin40
```
