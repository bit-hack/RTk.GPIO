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
 *
 * returns - true if GPIO board is currently open and attached.
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
void gpio_write(int pin, int state);

/**
 * Read the digital logic level on an input GPIO pin.
 *
 * arg pin - the GPIO pin to read.
 *
 * returns - 1 if a digital logic level high was read or 0 for low.
 */
int gpio_read(int pin);

/**
 * Set the pull up or pull down state of a pin.
 * 
 * arg pin   - the pin to set the pull state of.
 * arg state - set to 0 pull pin low, 1 to pull to up and -1 for no pull.
 */
void gpio_pull(int pin, int state);

/**
 * Setup pins for use as a SPI interface
 *
 * arg sck  - the GPIO pin that will act as the SPI clock.
 * arg mosi - the GPIO pin that will act as the 'master out slave in' pin.
 * arg miso - the GPIO pin that will act as the 'master in shave out' pin.
 * arg cs   - the GPIO pin that will act as the chip select pin (optional).
 */
void spi_init(int sck, int mosi, int miso, int cs=-1);

/**
 * Perform a SPI data transfer from the GPIO board.
 *
 * arg sck  - the GPIO pin that will act as the SPI clock.
 * arg mosi - the GPIO pin that will act as the 'master out slave in' pin.
 * arg miso - the GPIO pin that will act as the 'master in shave out' pin.
 * arg data - the data that will be transfered to the slave.
 * arg cs   - the GPIO pin that will act as the chip select pin (optional).
 *
 * returns - data received by the GPIO board during the SPI transaction.
 */
uint8_t spi_send(int sck, int mosi, int miso, uint8_t data, int cs=-1);

/**
 * Query the RTk.GPIO board firmware version
 *
 * arg dst      - destination buffer for version string.
 * arg dst_size - size of the destination buffer.
 */
void gpio_board_version(char* dst, uint32_t dst_size);

#ifdef __cplusplus
}  // extern "C"
#endif
