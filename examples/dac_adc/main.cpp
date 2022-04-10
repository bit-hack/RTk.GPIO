#include <stdio.h>
#include "gpio.h"

enum {
  PIN_CE_MCP4802 = 7,  // DAC  CE1
  PIN_CE_MCP3202 = 8,  // ADC  CE0
};

#define USE_HW_SPI 1

static uint8_t spi_send(uint8_t data) {
#if USE_HW_SPI
  return spi_hw_send(data);
#else
  return spi_sw_send(data);
#endif
}

static void mcp4802_write(int channel, bool gain_2x, bool shutdown, uint16_t value) {

  gpio_write(PIN_CE_MCP4802, 0);

  const uint8_t word0 =
    (channel  == 0     ? 0x00 : 0x80) |
    (gain_2x  == false ? 0x00 : 0x20) |
    (shutdown == true  ? 0x00 : 0x10) |
    (value >> 8) &              0x0f;
  spi_send(word0);

  const uint8_t word1 = value & 0xff;
  spi_send(word1);

  gpio_write(PIN_CE_MCP4802, 1);
}

static uint16_t mcp3202_read(int channel) {

  gpio_write(PIN_CE_MCP3202, 0);

  const uint8_t word0 = 0x01;                                   // start
  const uint8_t word1 = 0x80 | (channel ? 0x40 : 0x00) | 0x20;  // single ended, set channel, msb first
  const uint8_t word2 = 0x00;                                   // dummy byte

  const uint8_t recv0 = spi_send(word0);
  const uint8_t recv1 = spi_send(word1);
  const uint8_t recv2 = spi_send(word2);

  (void)recv0;

  gpio_write(PIN_CE_MCP3202, 1);

  return ((recv1 & 0x0f) << 8) | recv2;
}

int main(int argc, char** args) {

  if (!gpio_open(nullptr)) {
    return 1;
  }

  // print board version
  char version[32] = "\0";
  gpio_board_version(version, sizeof(version));
  printf("version: %s\n", version);

  // setup the pins
  if (!USE_HW_SPI) {
    spi_sw_init();
  }
  gpio_output(PIN_CE_MCP3202);
  gpio_write(PIN_CE_MCP3202, 1);
  gpio_output(PIN_CE_MCP4802);
  gpio_write(PIN_CE_MCP4802, 1);

  // itterate a number of times
  for (int i = 0; i < 1024 * 1024; ++i) {

    uint16_t write0 = i & 0xfff;
    mcp4802_write(0, false, false, write0);

    uint16_t write1 = 0xfff - (i & 0xfff);
    mcp4802_write(1, false, false, write1);

    uint16_t read0 = mcp3202_read(0);

    uint16_t read1 = mcp3202_read(1);

    printf("w0:%03x w1:%03x r0:%03x r1:%03x\n", write0, write1, read0, read1);
  }

  return 0;
}
