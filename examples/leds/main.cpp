#include <thread>
#include <chrono>
#include <stdio.h>
#include "gpio.h"

static const uint32_t pins[] = {
  24,  // LED0
  25,  // LED1
  8,   // LED2
  7,   // LED3
  14,  // LED4
  15,  // LED5
  18,  // LED6
  23,  // LED7
};

static void write_leds(uint8_t x) {
  for (uint32_t i = 0; i < 8; ++i) {
    const uint32_t bit = ((1 << i) & x) ? 0 : 1;
    gpio_write(pins[i], bit);
  }
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

  // default all LEDs to off
  for (uint32_t i = 0; i < 8; ++i) {
    gpio_output(pins[i]);
    gpio_write(pins[i], 1);
  }

  // itterate a number of times
  for (int i = 0; i < 1024 * 1024; ++i) {
    write_leds(i);
    gpio_delay(100);
  }

  return 0;
}
