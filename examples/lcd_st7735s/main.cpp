#include <thread>
#include <chrono>
#include <stdio.h>
#include "gpio.h"


enum {
  ST7735_TFTWIDTH  = 80,
  ST7735_TFTHEIGHT = 160,
  ST7735_COLS      = 132,
  ST7735_ROWS      = 162,
};

enum {
  ST7735_NOP     = 0x00,
  ST7735_SWRESET = 0x01,
  ST7735_RDDID   = 0x04,
  ST7735_RDDST   = 0x09,
  ST7735_SLPIN   = 0x10,
  ST7735_SLPOUT  = 0x11,
  ST7735_PTLON   = 0x12,
  ST7735_NORON   = 0x13,
  ST7735_INVOFF  = 0x20,
  ST7735_INVON   = 0x21,
  ST7735_DISPOFF = 0x28,
  ST7735_DISPON  = 0x29,
  ST7735_CASET   = 0x2A,
  ST7735_RASET   = 0x2B,
  ST7735_RAMWR   = 0x2C,
  ST7735_RAMRD   = 0x2E,
  ST7735_PTLAR   = 0x30,
  ST7735_MADCTL  = 0x36,
  ST7735_COLMOD  = 0x3A,
  ST7735_FRMCTR1 = 0xB1,
  ST7735_FRMCTR2 = 0xB2,
  ST7735_FRMCTR3 = 0xB3,
  ST7735_INVCTR  = 0xB4,
  ST7735_DISSET5 = 0xB6,
  ST7735_PWCTR1  = 0xC0,
  ST7735_PWCTR2  = 0xC1,
  ST7735_PWCTR3  = 0xC2,
  ST7735_PWCTR4  = 0xC3,
  ST7735_PWCTR5  = 0xC4,
  ST7735_VMCTR1  = 0xC5,
  ST7735_RDID1   = 0xDA,
  ST7735_RDID2   = 0xDB,
  ST7735_RDID3   = 0xDC,
  ST7735_RDID4   = 0xDD,
  ST7735_GMCTRP1 = 0xE0,
  ST7735_GMCTRN1 = 0xE1,
  ST7735_PWCTR6  = 0xFC,
};

enum {
  PIN_CS = 2,  // chip select
  PIN_DC = 3,  // data/control
  PIN_BL = 4,  // backlight
};

static void st7735_data(uint8_t data) {
  gpio_write(PIN_DC, 1);
  spi_hw_send(data);
}

static void st7735_cmd(uint8_t cmd) {
  gpio_write(PIN_DC, 0);
  spi_hw_send(cmd);
}

static void st7735_init() {

  gpio_write(PIN_CS, 0);

  st7735_cmd(ST7735_SWRESET);       // Software reset
  gpio_delay(150);                  // gpio_delay 150 ms

  st7735_cmd(ST7735_SLPOUT);        // Out of sleep mode
  gpio_delay(500);                  // gpio_delay 500 ms

  st7735_cmd(ST7735_FRMCTR1);       // Frame rate ctrl - normal mode
  st7735_data(0x01);                // Rate = fosc / (1x2 + 40) * (LINE + 2C + 2D)
  st7735_data(0x2C);
  st7735_data(0x2D);

  st7735_cmd(ST7735_FRMCTR2);       // Frame rate ctrl - idle mode
  st7735_data(0x01);                // Rate = fosc / (1x2 + 40) * (LINE + 2C + 2D)
  st7735_data(0x2C);
  st7735_data(0x2D);

  st7735_cmd(ST7735_FRMCTR3);       // Frame rate ctrl - partial mode
  st7735_data(0x01);                // Dot inversion mode
  st7735_data(0x2C);
  st7735_data(0x2D);
  st7735_data(0x01);                // Line inversion mode
  st7735_data(0x2C);
  st7735_data(0x2D);

  st7735_cmd(ST7735_INVCTR);        // Display inversion ctrl
  st7735_data(0x07);                // No inversion

  st7735_cmd(ST7735_PWCTR1);        // Power control
  st7735_data(0xA2);
  st7735_data(0x02);                //  - 4.6V
  st7735_data(0x84);                // auto mode

  st7735_cmd(ST7735_PWCTR2);        // Power control
  st7735_data(0x0A);                // Opamp current small
  st7735_data(0x00);                // Boost frequency

  st7735_cmd(ST7735_PWCTR4);        // Power control
  st7735_data(0x8A);                // BCLK / 2, Opamp current small & Medium low
  st7735_data(0x2A);

  st7735_cmd(ST7735_PWCTR5);        // Power control
  st7735_data(0x8A);
  st7735_data(0xEE);

  st7735_cmd(ST7735_VMCTR1);        // Power control
  st7735_data(0x0E);

  st7735_cmd(ST7735_INVON);         // Invert display

  st7735_cmd(ST7735_MADCTL);        // Memory access control(directions)
  st7735_data(0xC8);                // row addr / col addr, bottom to top refresh

  st7735_cmd(ST7735_COLMOD);        // set color mode
  st7735_data(0x05);                // 16 - bit color

  st7735_cmd(ST7735_CASET);         // Column addr set
  st7735_data(0x00);                // XSTART = 0
  st7735_data(0);
  st7735_data(0x00);                // XEND = ROWS - height
  st7735_data(ST7735_TFTWIDTH + 0 - 1);

  st7735_cmd(ST7735_RASET);         // Row addr set
  st7735_data(0x00);                // YSTART = 0
  st7735_data(0);
  st7735_data(0x00);                // YEND = COLS - width
  st7735_data(ST7735_TFTHEIGHT + 0 - 1);

  st7735_cmd(ST7735_GMCTRP1);       // Set Gamma
  st7735_data(0x02);
  st7735_data(0x1c);
  st7735_data(0x07);
  st7735_data(0x12);
  st7735_data(0x37);
  st7735_data(0x32);
  st7735_data(0x29);
  st7735_data(0x2d);
  st7735_data(0x29);
  st7735_data(0x25);
  st7735_data(0x2B);
  st7735_data(0x39);
  st7735_data(0x00);
  st7735_data(0x01);
  st7735_data(0x03);
  st7735_data(0x10);

  st7735_cmd(ST7735_GMCTRN1);       // Set Gamma
  st7735_data(0x03);
  st7735_data(0x1d);
  st7735_data(0x07);
  st7735_data(0x06);
  st7735_data(0x2E);
  st7735_data(0x2C);
  st7735_data(0x29);
  st7735_data(0x2D);
  st7735_data(0x2E);
  st7735_data(0x2E);
  st7735_data(0x37);
  st7735_data(0x3F);
  st7735_data(0x00);
  st7735_data(0x00);
  st7735_data(0x02);
  st7735_data(0x10);

  st7735_cmd(ST7735_NORON);         // Normal display on
  gpio_delay(10);                   // 10 ms

  st7735_cmd(ST7735_DISPON);        // Display on
  gpio_delay(100);                  // 100 ms

  gpio_write(PIN_CS, 1);
}

static void st7735_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {

  const int xoffs = (ST7735_COLS - ST7735_TFTWIDTH)  / 2;
  const int yoffs = (ST7735_ROWS - ST7735_TFTHEIGHT) / 2;
  x0 += xoffs;
  x1 += xoffs;
  y0 += yoffs;
  y1 += yoffs;

  gpio_write(PIN_CS, 0);

  st7735_cmd(ST7735_CASET);  // Column addr set
  st7735_data(x0 >> 8);
  st7735_data(x0 & 0xff);    // XSTART
  st7735_data(x1 >> 8);
  st7735_data(x1 & 0xff);    // XEND

  st7735_cmd(ST7735_RASET);  // Row addr set
  st7735_data(y0 >> 8);
  st7735_data(y0 & 0xff);    // YSTART
  st7735_data(y1 >> 8);
  st7735_data(y1 & 0xff);    // YEND
  st7735_cmd(ST7735_RAMWR);  // write to RAM

  gpio_write(PIN_CS, 1);
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

  gpio_output(PIN_BL);
  gpio_write(PIN_BL, 1);

  gpio_output(PIN_CS);
  gpio_write(PIN_CS, 1);

  gpio_output(PIN_DC);

  st7735_init();

  // Uses 16bit colour RBG (565) format
  //
  // 0b1111100000000000;  // R
  // 0b0000011111100000;  // G
  // 0b0000000000011111;  // B

  for (;;) {

    st7735_window(0, 0, ST7735_TFTWIDTH - 1, ST7735_TFTHEIGHT - 1);

    for (int y = 0; y < ST7735_TFTHEIGHT; ++y) {
      for (int x = 0; x < ST7735_TFTWIDTH; ++x) {

        const uint64_t c = (y & 0x1f) | ((x & 0x3f) << 5);

        gpio_write(PIN_CS, 0);
        st7735_data((c >> 8) & 0xff);
        st7735_data(c & 0xff);
        gpio_write(PIN_CS, 1);
      }
    }

    st7735_window(0, 0, ST7735_TFTWIDTH - 1, ST7735_TFTHEIGHT - 1);

    for (int y = 0; y < ST7735_TFTHEIGHT; ++y) {
      for (int x = 0; x < ST7735_TFTWIDTH; ++x) {

        const uint64_t c = (x & 0x1f) | ((y & 0x3f) << 5);

        gpio_write(PIN_CS, 0);
        st7735_data((c >> 8) & 0xff);
        st7735_data(c & 0xff);
        gpio_write(PIN_CS, 1);
      }
    }
  }

  return 0;
}
