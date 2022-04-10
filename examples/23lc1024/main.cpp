#include <thread>
#include <chrono>
#include <stdio.h>
#include "gpio.h"

//        23lc1024
//
//         .--_--.
//  /cs   -|     |-   3v3
//  SO    -|     |-   /HOLD
//        -|     |-   SCK
//  gnd   -|     |-   SI
//         '-----'  
//

#define hardware_spi 0

enum {
  pin_cs = 2,
};

enum {
  CMD_READ        = 0x03,
  CMD_WRITE       = 0x02,
  CMD_RDMR        = 0x05,
  CMD_WRMR        = 0x01,
  MODE_BYTE       = 0x00,
  MODE_PAGE       = 0x80,
  MODE_SEQUENTIAL = 0x40,  // default
  MAX_ADDR        = 0x20000
};


static uint8_t spi_send(uint8_t data) {
  if (hardware_spi) {
    return spi_hw_send(data);
  }
  else {
    return spi_sw_send(data);
  }
}

uint8_t sram_read(uint32_t addr) {
  gpio_write(pin_cs, gpio_low);
  spi_send(CMD_READ);
  spi_send((addr >> 16) & 0xff);
  spi_send((addr >>  8) & 0xff);
  spi_send((addr >>  0) & 0xff);
  const uint8_t data = spi_send(0xff);
  gpio_write(pin_cs, gpio_high);
  return data;
}

void sram_write(uint32_t addr, uint8_t data) {
  gpio_write(pin_cs, gpio_low);
  spi_send(CMD_WRITE);
  spi_send((addr >> 16) & 0xff);
  spi_send((addr >>  8) & 0xff);
  spi_send((addr >>  0) & 0xff);
  spi_send(data);
  gpio_write(pin_cs, gpio_high);
}

uint8_t sram_read_mode(void) {
  gpio_write(pin_cs, gpio_low);
  spi_send(CMD_RDMR);
  const uint8_t data = spi_send(0xff);
  gpio_write(pin_cs, gpio_high);
  return data;
}

void sram_write_mode(uint8_t data) {
  gpio_write(pin_cs, gpio_low);
  spi_send(CMD_WRMR);
  spi_send(data);
  gpio_write(pin_cs, gpio_high);
}

static uint8_t random(uint32_t &x) {
  x ^= x << 13;
  x ^= x >> 17;
  x ^= x << 5;
  return x & 0xff;
}

int main(int argc, char** args) {

  // open RTk.GPIO connection
  if (!gpio_open(nullptr)) {
    return 1;
  }

  // print board version
  char version[32] = "\0";
  gpio_board_version(version, sizeof(version));
  printf("version: %s\n", version);

  // setup software spi if needed
  if (!hardware_spi) {
    spi_sw_init();
  }

  // pull CS high
  gpio_output(pin_cs);
  gpio_write(pin_cs, gpio_high);

  // read the current mode register
  uint8_t mode = sram_read_mode();

  uint32_t rng1 = 12345;
  uint32_t rng2 = 12345;

  for (int j = 0; j < MAX_ADDR; j += 1024) {

    for (int i = 0; i < 1024; ++i) {
      sram_write(j + i, random(rng1));
      printf("w %d %02x\n", j+i, (i & 0xff));
    }

    for (int i = 0; i < 1024; ++i) {
      uint8_t got = sram_read(j + i);
      uint8_t expect = random(rng2);
      printf("r %d %02x (%02x) %c\n", j+i, got, expect, got == expect ? ' ' : '!');
    }

  }

  return 0;
}
