# RTk.GPIO C++ Interface Library

This is a C++ interface library for controlling the RTk.GPIO board.
The board provides a RaspberryPi like GPIO interface to computers via a USB to Serial chip.

RTk.GPIO boards are available from [https://pi-supply.com](https://pi-supply.com).


### Available APIs

This library provides two c++ APIs for controlling the RTk.GPIO board:
- A gpio interface (gpio.h).
- A WiringPi interface (WiringPi.h, WiringPiSPI.h).

Both are very similar and infact the WiringPi interface is implemented directly on top of the gpio interface.
The WiringPi interface, provides a subset of the WiringPI API, and was added simply to make it easy to port software between platforms.
Not all WiringPi functions are available however due to limitations of the RTk.GPIO board, so it will not work for all applications.


### RTK.GPIO pinout

The GPIO pinout is as follows:
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


### The RTk.GPIO protocol

The RTk.GPIO boards use a very simple ascii based protocol operating at `230400` baud.
Because of the serial based nature of the interface its not possible to operate at high speeds.

In all cases the computer acts as a master issuing commands, and the RTk board operates as a slave, acting on received commands.

GPIO pins are identified using lower case letters beginning with `a` which represents GP0, `b` which represents GP1, etc.

A pin number is followed by a command to inform the interface to perform an action.
Examples of supported commands are as follows:

- `aO` instructs the interface to make the GP0 pin an output (low impedance).
- `bI` instructs the interface to make the GP1 pin an input (high impedance).
- `c0` drives the GP2 pin to logic level low.
- `d1` drives the GP3 pin to logic level high.
- `e?` reads the current logic level on the GP4 pin.

The `?` command will produce a response from the board in the following format `x0\r\n` or `x1\r\n` where `x` is the pin being read.
If an output pin is read from, it will return its current driving logic level.
