#include "mbed.h"

#define BAUD_RATE   230400
#define PIN_COUNT   28
#define VERSION_STR "RTk.GPIO v2 07/04/2022"
#define READY_STR   "RTk.GPIO Ready"


// pins default to Input and PullNone
static DigitalInOut gp[] = {
    DigitalInOut(PA_1 ), // GP0
    DigitalInOut(PB_12), // GP1
    DigitalInOut(PB_7 ), // GP2
    DigitalInOut(PB_6 ), // GP3
    DigitalInOut(PA_8 ), // GP4
    DigitalInOut(PA_12), // GP5
    DigitalInOut(PA_13), // GP6
    DigitalInOut(PF_1 ), // GP7
    DigitalInOut(PB_5 ), // GP8
    DigitalInOut(PA_6 ), // GP9
    DigitalInOut(PA_7 ), // GP10
    DigitalInOut(PA_5 ), // GP11
    DigitalInOut(PF_0 ), // GP12
    DigitalInOut(PA_14), // GP13
    DigitalInOut(PA_2 ), // GP14
    DigitalInOut(PA_3 ), // GP15
    DigitalInOut(PB_8 ), // GP16
    DigitalInOut(PB_15), // GP17
    DigitalInOut(PB_1 ), // GP18
    DigitalInOut(PA_15), // GP19
    DigitalInOut(PB_9 ), // GP20
    DigitalInOut(PB_10), // GP21
    DigitalInOut(PA_11), // GP22
    DigitalInOut(PB_2 ), // GP23
    DigitalInOut(PB_3 ), // GP24
    DigitalInOut(PB_4 ), // GP25
    DigitalInOut(PB_0 ), // GP26
    DigitalInOut(PB_14)  // GP27
};

static Serial serialPort(/*TX=*/PA_9, /*RX=*/PA_10);

// enhanced mode enables fast paths in the code and extended feature support.
static bool enhanced_mode = false;

// index of the next pin operation
static uint8_t latched_pin = 0;

// perform a pin related action
static void dispatch_pin(uint8_t pin, uint8_t op) {
    switch (op) {
    case '0':
        gp[pin] = 0;
        break;
    case '1':
        gp[pin] = 1;
        break;
    case 'O':
        gp[pin].output();
        break;
    case 'I':
        gp[pin].input();
        if (!enhanced_mode) {
            gp[pin].mode(PullNone);
        }
        break;
    case 'D':
        gp[pin].mode(PullDown);
        break;
    case 'U':
        gp[pin].mode(PullUp);
        break;
    case 'N':
        gp[pin].mode(PullNone);
        break;
    case '?':
        serialPort.putc('a' + pin);
        serialPort.putc( gp[pin].read() ? '1' : '0' );
        // these operations are redundant in enhanced mode
        if (!enhanced_mode) {
            serialPort.putc('\r');
            serialPort.putc('\n');
        }
        break;
    }
}

// return a RTk.GPIO firmware version string
static void dispatch_version(void) {
    serialPort.puts(VERSION_STR);
}

// increment and wrap the latched pin index
static void inc_latched_pin(void) {
    ++latched_pin;
    if (latched_pin >= PIN_COUNT) {
        latched_pin = 0;
    }
}

// convert 6 bit value to ascii base64 character
static char base64(uint64_t x) {
    if (           x <= 25) { return 'a' +  x;       }
    if (x >= 26 && x <= 51) { return 'A' + (x - 26); }
    if (x >= 52 && x <= 61) { return '0' + (x - 52); }
    return '!' + (x - 62);
}

// convert ascii base64 character to 6 bit value
static uint8_t debase64(char x) {
    if (x >= 'a' && x <= 'z') { return uint8_t(x - 'a');      }
    if (x >= 'A' && x <= 'Z') { return uint8_t(x - 'A' + 26); }
    if (x >= '0' && x <= '9') { return uint8_t(x - '0' + 52); }
    return (x - '!') + 62;
}

// read 6 input pins at once
static void bulk_input() {
    uint8_t out = 0;
    for (int i=0; i<6; ++i) {
        out = (out >> 1) | (gp[latched_pin].read() ? 0x20 : 0x00);
        inc_latched_pin();
    }
    serialPort.putc(base64(out));
}

// write 6 output pins at once
static void bulk_output() {
    uint8_t dat = debase64(serialPort.getc());
    for (int i=0; i<6; ++i) {
        gp[latched_pin] = dat & 1;
        dat >>= 1;
        inc_latched_pin();
    }
}

// perform a given action
static void dispatch(const char dat) {

    // pin adjustment
    if (dat >= 'a' && dat < ('a' + PIN_COUNT)) {
        const uint8_t pin = uint8_t(dat - 'a');        
        // latch this pin for later operations
        latched_pin = pin;

        // the original functionality will take a single bit following immediately
        // but in enhanced mode we latch the pin and wait for the next operation
        if (!enhanced_mode) {
            // get pin operation
            const char op = serialPort.getc();
            // perform it
            dispatch_pin(/*pin=*/pin, /*op=*/op);
        }
        return;
    }

    // enhanced mode operations
    if (enhanced_mode) {

        // latched pin adjustment
        if (dat == '0' || dat == '1' ||
            dat == 'I' || dat == 'O' ||
            dat == '?' ||
            dat == 'U' || dat == 'D' || dat == 'N') {
            dispatch_pin(/*pin=*/latched_pin, /*op=*/dat);
            inc_latched_pin();
            return;
        }

        // bulk input
        if (dat == '>') {
            bulk_input();
            return;
        }

        // bulk output
        if (dat == '<') {
            bulk_output();
            return;
        }
    }

    // version string
    if (dat == 'V') {
        dispatch_version();
        return;
    }

    // enter enhanced mode
    if (dat == '#') {
        enhanced_mode = true;
        return;
    }
}

int main() {

    // setup the serial port
    serialPort.baud(BAUD_RATE);
    serialPort.format(8, mbed::SerialBase::None, 1);
    serialPort.printf(READY_STR);

    // main loop
    for (;;) {

        // wait for data to be available
        if (!serialPort.readable()) {
            continue;
        }
        const char dat = (char)serialPort.getc();

        // dispatch based on mode
        dispatch(dat);
    }

    return 0;
}
