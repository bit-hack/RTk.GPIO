#include <thread>
#include <chrono>
#include <stdio.h>
#include "gpio.h"

int main(int argc, char** args) {

  // open RTk.GPIO connection
  if (!gpio_open(nullptr)) {
    return 1;
  }

  // print board version
  char version[32] = "\0";
  gpio_board_version(version, sizeof(version));
  printf("version: %s\n", version);

  // itterate a number of times
  for (int i = 0; i < 1024 * 1024; ++i) {

    uint32_t out = rand() & 0xff;

    uint32_t got = spi_hw_send(out);
    printf("%u: %u  %c\n", i, got, (out == got) ? ' ' : '!');
  }

  return 0;
}
