#pragma once

#include <assert.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Digital logic input pin (high impedance)
#define INPUT  0

// Digital logic output pin (low impedance)
#define OUTPUT 1

// Digital logic high level (~vcc)
#define HIGH 1

// Digital logic level low (~0v)
#define LOW 0

/**
 * Start the WiringPi library and connect to the GPIO board.
 * 
 * arg - The name of the com port the GPIO interface is attached to.
 *       See `gpio_open` for details.
 */
int wiringPiSetup(const char *port = NULL);

/**
 * Set the digital logic input or output state of a wiring pi pin.
 */
void pinMode(int pin, int state);

/**
 * Set the digital logic level of a wiring pi pin.
 */
void digitalWrite(int pin, int state);

/**
 * Read the digital logic level of a wiring pi pin.
 */
int digitalRead(int pin);

/**
 * Return the number of milliseconds elapsed since the program started.
 */
uint64_t millis();

/**
 * Suspend the program for a specified number of milliseconds.
 */
void delay(uint64_t ms);

/**
 * Suspend the program for a specified number of microseconds.
 */
void delayMicroseconds(uint64_t us);

#define wiringPiSetupGpio() \
  assert(!"wiringPiSetupGpio is not supported")

#define wiringPiSetupPhys() \
  assert(!"wiringPiSetupPhys is not supported")

#define wiringPiSetupSys () \
  assert(!"wiringPiSetupSys is not supported")

#define pullUpDnControl(pin, pud) \
  assert(!"pullUpDnControl is not supported")

#define pwmWrite(pin, value) \
  assert(!"pwmWrite is not supported")

#define analogRead(pin) \
  assert(!"analogRead is not supported")

#define analogWrite(pin, value) \
  assert(!"analogWrite is not supported")

#ifdef __cplusplus
}  // extern "C"
#endif
