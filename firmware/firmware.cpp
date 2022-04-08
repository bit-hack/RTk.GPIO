#include "mbed.h"

#define BAUD_RATE   230400
#define PIN_COUNT   28
#define VERSION_STR "RTk.GPIO v2 08/04/2022"
#define READY_STR   "RTk.GPIO Ready"

// pin number mapping
static const PinName gpPinMap[] = {
    PA_1 , PB_12, PB_7 , PB_6 , PA_8 , PA_12, PA_13, PF_1 ,
    PB_5 , PA_6 , PA_7 , PA_5 , PF_0 , PA_14, PA_2 , PA_3 ,
    PB_8 , PB_15, PB_1 , PA_15, PB_9 , PB_10, PA_11, PB_2 ,
    PB_3 , PB_4 , PB_0 , PB_14,
};
static DigitalInOut *gp[PIN_COUNT];

// SPI bus
static const PinName spiPinSck  = PA_5;  // gp11
static const PinName spiPinMiso = PA_6;  // gp9
static const PinName spiPinMosi = PA_7;  // gp10
static SPI *spi;

// UART serial port
static Serial serialPort(/*TX=*/PA_9, /*RX=*/PA_10);

// enhanced mode enables fast paths in the code and extended feature support.
static bool enhanced_mode = false;

// index of the next pin operation
static uint8_t latched_pin = 0;

// dispose of a bound gpio object
static void gpio_dispose(uint8_t pin) {
    if (gp[pin]) {
        delete gp[pin];
        gp[pin] = NULL;
    }
}

// dispose of a bound spi object
static void spi_dispose(void) {
    if (spi) {
        delete spi;
        spi = NULL;
    }
}

static DigitalInOut *gpio_get(uint8_t pin) {
    // check if the digital pin already exists
    if (gp[pin]) {
        return gp[pin];
    }
    // get the mbed pin
    const PinName mpin = gpPinMap[pin];
    // check if we conflict with the spi object
    const bool uses_spi = (mpin == spiPinMiso) ||
                          (mpin == spiPinMosi) ||
                          (mpin == spiPinSck);
    if (uses_spi) {
        spi_dispose();
    }
    // create the new GPIO object
    return gp[pin] = new DigitalInOut(mpin);
}

static SPI *spi_get() {
    // check if spi object already exists
    if (spi) {
        return spi;
    }
    // check if one of the SPI pins is currently used
    gpio_dispose(9);   // spiPinMiso
    gpio_dispose(10);  // spiPinMosi
    gpio_dispose(11);  // spiPinSck
    // create new SPI object
    spi = new SPI(spiPinMosi, spiPinMiso, spiPinSck);
    return spi;
}

static uint8_t hex_to_nibble(char x) {
    return (x >= '0' && x <= '9') ? (x - '0') : ((x - 'A') + 10);
}

static char nibble_to_hex(uint8_t x) {
    return (x >= 10) ? ('A' + (x - 10)) : ('0' + x);
}

static void dispatch_spi(uint8_t action) {
    SPI *spi = spi_get();
    // transfer spi byte
    if (action == '~') {
        // transfer byte in big endian format
        const uint8_t d0 = hex_to_nibble(serialPort.getc());  // msb
        const uint8_t d1 = hex_to_nibble(serialPort.getc());  // lsb
        const uint8_t send = d1 | (d0 << 4);
        const uint8_t recv = spi->write(send);
        serialPort.putc(nibble_to_hex((recv >> 4) & 0xf));  // msb
        serialPort.putc(nibble_to_hex((recv     ) & 0xf));  // lsb
    }
    // start spi interface
    if (action == 'S') {
        (void)spi;
    }
}

// perform a pin related action
static void dispatch_pin(uint8_t pin, uint8_t action) {
    if (pin >= PIN_COUNT) {
        return;
    }
    // fetch the IO object for this pin or create in demand
    DigitalInOut *io = gpio_get(pin);
    // dispatch the operation
    switch (action) {
    case '0': io->write(0);       break;
    case '1': io->write(1);       break;
    case 'O': io->output();       break;
    case 'D': io->mode(PullDown); break;
    case 'U': io->mode(PullUp);   break;
    case 'N': io->mode(PullNone); break;
    case 'I':
        io->input();
        if (!enhanced_mode) {
            io->mode(PullNone);
        }
        break;
    case '?':
        serialPort.putc('a' + pin);
        serialPort.putc( io->read() ? '1' : '0' );
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
        // SPI operation
        if (dat == 'S' || dat == '~') {
            dispatch_spi(dat);
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
}
