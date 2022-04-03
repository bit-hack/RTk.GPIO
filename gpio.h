#pragma once

#include <stdint.h>
#include <stdbool.h>

// This define will let programs check if they are being compiled for use with
// an RTk GPIO board.
#define RTk_GPIO 1

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Open the serial port the GPIO board is attached to.
 * 
 * arg port - The name of the com port the GPIO interface is attached to:
 *            on Windows for example "COM4".
 *            on Linux for example, "/dev/..."
 * 
 * note: `port` can be NULL to attempt to attempt to use the first available
 *       COM port.
 * 
 * returns - true if the serial port was opened successfully.
**/
bool gpio_open(char *port);

/**
 * Close the serial connection to the GPIO board.
**/
void gpio_close(void);

/**
 * Check if the GPIO interface has been opened.
**/
bool gpio_is_open(void);

/**
 * Set a GPIO pin to act as an input.
 * 
 * arg pin - the GPIO pin to set as an input.
 */
void gpio_input(int pin);

/**
 * Set a GPIO pin to act as an output.
 * 
 * arg pin - the GPIO pin to set as an output.
 */
void gpio_output(int pin);

/**
 * Set the digital logic level of an output GPIO pin.
 * 
 * arg pin   - the GPIO pin to set the output level of.
 * arg state - if set to 0 the pin will be voltage level low, otherwise high.
 */
void gpio_write(int pin, bool state);

/**
 * Read the digital logic level on an input GPIO pin.
 * 
 * arg pin - the GPIO pin to read.
 * 
 * returns - true if a digital logic level high was read or false for low.
 */
bool gpio_read(int pin);

/**
 * Perform a SPI data transfer from the GPIO board.
 *
 * arg sck  - the GPIO pin that will act as the SPI clock.
 * arg mosi - the GPIO pin that will act as the 'master out slave in' pin.
 * arg data - the data that will be transfered to the slave.
 * arg cs   - the GPIO pin that will act as the chip select pin. (optional)
 * 
 * returns - data received by the GPIO board during the SPI transaction.
 */
uint8_t spi_send(int sck, int mosi, int miso, uint8_t data, int cs=-1);

#ifdef __cplusplus
}  // extern "C"
#endif