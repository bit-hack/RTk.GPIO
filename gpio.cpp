#ifdef __cplusplus
#include <thread>
#include <chrono>
#endif

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

//-----------------------------------------------------------------------------
// WINDOWS SERIAL
//-----------------------------------------------------------------------------

#if defined(_MSC_VER)

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

struct serial_t {
  HANDLE handle;
};

static BOOL set_timeouts(HANDLE handle) {
  COMMTIMEOUTS com_timeout;
  ZeroMemory(&com_timeout, sizeof(com_timeout));
  com_timeout.ReadIntervalTimeout = 3;
  com_timeout.ReadTotalTimeoutMultiplier = 3;
  com_timeout.ReadTotalTimeoutConstant = 2;
  com_timeout.WriteTotalTimeoutMultiplier = 3;
  com_timeout.WriteTotalTimeoutConstant = 2;
  return SetCommTimeouts(handle, &com_timeout);
}

static bool get_port_name(const char* port, char* out, size_t size) {

  // make out an empty string by default
  *out = '\0';

  // use a user supplied com port name
  if (port) {
    snprintf(out, size, "\\\\.\\%s", port);
    return true;
  }

  // find the first available COM port
  for (int i = 0; i < 256; ++i) {
    char temp[16];
    snprintf(temp, sizeof(temp), "COM%d", i);
    HANDLE h = CreateFile(temp, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (h != INVALID_HANDLE_VALUE) {
      CloseHandle(h);
      snprintf(out, size, "\\\\.\\COM%d", i);
      return true;
    }
  }

  // no success
  return false;
}

static serial_t* serial_open(const char *port, uint32_t baud_rate) {
  // construct com port device name
  char dev_name[32];
  if (!get_port_name(port, dev_name, sizeof(dev_name))) {
    return NULL;
  }
  // open handle to serial device
  HANDLE handle = CreateFileA(
    dev_name,
    GENERIC_READ | GENERIC_WRITE,
    0,
    NULL,
    OPEN_EXISTING,
    0,
    NULL);
  if (handle == INVALID_HANDLE_VALUE) {
    goto on_error;
  }
  // query serial device control block
  DCB dbc;
  ZeroMemory(&dbc, sizeof(dbc));
  dbc.DCBlength = sizeof(dbc);
  if (GetCommState(handle, &dbc) == FALSE) {
    goto on_error;
  }
  // change baud rate
  if (dbc.BaudRate != baud_rate) {
    dbc.BaudRate = baud_rate;
  }
  dbc.fBinary = TRUE;
  dbc.fParity = FALSE;
  dbc.fOutxCtsFlow = FALSE;
  dbc.fDtrControl = FALSE;
  dbc.ByteSize = 8;
  dbc.fOutX = FALSE;
  dbc.fInX = FALSE;
  dbc.fNull = FALSE;
  dbc.fRtsControl = RTS_CONTROL_DISABLE;
  dbc.Parity = NOPARITY;
  dbc.StopBits = ONESTOPBIT;
  dbc.EofChar = 0;
  dbc.ErrorChar = 0;
  dbc.EvtChar = 0;
  dbc.XonChar = 0;
  dbc.XoffChar = 0;

  if (SetCommState(handle, &dbc) == FALSE) {
    goto on_error;
  }
  // set com timeouts
  if (set_timeouts(handle) == FALSE) {
    goto on_error;
  }
  // wrap in serial object
  serial_t* serial = (serial_t*)malloc(sizeof(serial_t));
  if (serial == NULL) {
    goto on_error;
  }
  serial->handle = handle;
  // success
  return serial;
  // error handler
on_error:
  if (handle != INVALID_HANDLE_VALUE)
    CloseHandle(handle);
  return NULL;
}

static void serial_close(serial_t* serial) {
  assert(serial);
  if (serial->handle != INVALID_HANDLE_VALUE) {
    CloseHandle(serial->handle);
  }
  free(serial);
}

static uint32_t serial_send(serial_t* serial, const void* data, size_t nbytes) {
  assert(serial && data && nbytes);
  DWORD nb_written = 0;
  if (WriteFile(
    serial->handle,
    data,
    (DWORD)nbytes,
    &nb_written,
    NULL) == FALSE) {
    return 0;
  }
  return nb_written;
}

static uint32_t serial_read(serial_t* serial, void* dst, size_t nbytes) {
  assert(serial && dst && nbytes);
  DWORD nb_read = 0;
  if (ReadFile(
    serial->handle,
    dst,
    (DWORD)nbytes,
    &nb_read,
    NULL) == FALSE) {
    return 0;
  }
  return nb_read;
}

static void serial_flush(serial_t* serial) {
  FlushFileBuffers(serial->handle);
}

#endif  // defined(_MSC_VER)

//-----------------------------------------------------------------------------
// LINUX SERIAL
//-----------------------------------------------------------------------------

#if !defined(_MSC_VER)

// TODO

#endif  // !defined(_MSC_VER)

//-----------------------------------------------------------------------------
// GPIO
//-----------------------------------------------------------------------------

#define CHECK_PIN(PIN) \
  assert(PIN >= 0 && PIN <= 27)

struct state_t {
  bool enhanced_mode;
  uint32_t latched_pin;

  // todo: cache state
};

static state_t state;
static serial_t* serial;

static void latched_pin_inc() {
  ++state.latched_pin;
  if (state.latched_pin >= 27) {
    state.latched_pin = 0;
  }
}

static void gpio_set_pin(int pin) {
  CHECK_PIN(pin);
  if (serial) {
    char data = 'a' + char(pin);
    // early exit if bin already bound
    if (state.enhanced_mode) {
      if (state.latched_pin == pin) {
        return;
      }
    }
    // explicitly set the pin
    serial_send(serial, &data, 1);
    state.latched_pin = pin;
  }
}

static void gpio_action(int pin, char action) {
  CHECK_PIN(pin);
  if (serial) {
    // set the pin
    gpio_set_pin(pin);
    // perform the action
    serial_send(serial, &action, 1);
    latched_pin_inc();
  }
}

extern "C" {

bool gpio_open(const char *port) {

  if (serial) {
    serial_close(serial);
    serial = NULL;
  }

  // open serial connection
  const int baud = 230400;
  serial = serial_open(port, baud);
  if (!serial) {
    return false;
  }

  // clear the input buffer
  uint8_t send[4] = { ' ', ' ', ' ', ' ' };
  serial_send(serial, send, 2);
  serial_read(serial, send, 4);

  // try to start enhanced mode
  serial_send(serial, "#", 1);
  uint8_t out[] = { 0, 0 };
  serial_read(serial, out, 2);
  if (out[0] != 'E') {
    state.enhanced_mode = true;
    state.latched_pin   = 0;
  }

  return true;
}

bool gpio_is_open(void) {
  return serial != NULL;
}

void gpio_close(void) {
  if (serial) {
    serial_close(serial);
    serial = nullptr;
  }
}

void gpio_input(int pin) {
  CHECK_PIN(pin);
  gpio_action(pin, 'I');
}

void gpio_output(int pin) {
  CHECK_PIN(pin);
  gpio_action(pin, 'O');
}

void gpio_write(int pin, int state) {
  CHECK_PIN(pin);
  gpio_action(pin, state ? '1' : '0');
}

void gpio_pull(int pin, int state) {
  CHECK_PIN(pin);
  const char action = (state == 1) ? 'U' :
                      (state == 0) ? 'D' :
                                     'N';
  gpio_action(pin, action);
}

int gpio_read(int pin) {
  CHECK_PIN(pin);
  gpio_action(pin, '?');
  char data[4] = { 0 };
  serial_read(serial, data, state.enhanced_mode ? 2 : 4);
  return (data[1] == '1') ? 1 : 0;
}

void gpio_board_version(char* dst, uint32_t dst_size) {
  assert(dst && dst_size);
  if (serial) {
    const char* end = dst + (dst_size - 1);
    // request the version string
    // note: we send two bytes but the second is ignored but required by the firmware
    serial_send(serial, "V_", state.enhanced_mode ? 1 : 2);

    // while we have more space
    for (; dst < end; ++dst) {
      char recv = '\0';
      serial_read(serial, &recv, 1);
      // exit on new line or carage return
      if (recv == '\r' || recv == '\n' || recv == '\0') {
        break;
      }
      // append character
      *dst = recv;
    }
  }
  // append trailing zero
  *dst = '\0';
}

void spi_init(int sck, int mosi, int miso, int cs) {

  CHECK_PIN(sck);
  CHECK_PIN(mosi);
  CHECK_PIN(miso);

  if (cs >= 0 && cs <= 27) {
    gpio_output(cs);
    gpio_write(cs, 1);  // cs high (not asserted)
  }

  gpio_output(mosi);
  gpio_write (mosi, 1);
  gpio_output(sck);
  gpio_write (sck,  1);
  gpio_input (miso);
}

uint8_t spi_send(int sck, int mosi, int miso, uint8_t data, int cs) {

  CHECK_PIN(sck);
  CHECK_PIN(mosi);
  CHECK_PIN(miso);

  // pull CS low
  if (cs >= 0 && cs <= 27)
    gpio_write(cs, 0);

  uint8_t recv = 0;
  for (int i = 0; i < 8; ++i) {
    // clock goes low
    gpio_write(sck, 0);
    // shift data out
    gpio_write(mosi, (data & 0x80) ? 1 : 0);
    data = (data << 1);
    // clock goes high
    gpio_write(sck, 1);
    // shift new data in
    recv = (recv << 1) | (gpio_read(miso) ? 1 : 0);
  }

  // pull CS high
  if (cs >= 0 && cs <= 27)
    gpio_write(cs, 1);

  return recv;
}

static uint8_t hex_to_nibble(char x) {
  return (x >= '0' && x <= '9') ? (x - '0') : ((x - 'A') + 10);
}

static char nibble_to_hex(uint8_t x) {
  return (x >= 10) ? ('A' + (x - 10)) : ('0' + x);
}

uint8_t spi_send_hw(uint8_t data, int cs = -1) {

  if (!state.enhanced_mode) {
    return 0;
  }

  // pull CS low
  if (cs >= 0 && cs <= 27)
    gpio_write(cs, 0);

  // send byte to transmit
  serial_send(serial, "~", 1);
  const char out[2] = {
    nibble_to_hex((data & 0xf0) >> 4),
    nibble_to_hex((data & 0x0f))
  };
  serial_send(serial, out, sizeof(out));

  // send byte to receive
  char dst[2] = { 0, 0 };
  serial_read(serial, dst, 2);

  const uint8_t ret = (hex_to_nibble(dst[0]) << 4) |
                       hex_to_nibble(dst[1]);

  // pull CS high
  if (cs >= 0 && cs <= 27)
    gpio_write(cs, 1);

  return ret;
}

}  // extern "C"

//-----------------------------------------------------------------------------
// WIRING PI WRAPPER
//-----------------------------------------------------------------------------

// convert wiring pi pin numbers to gpio pin numbers
static int wpi_pin(int pin) {
  switch (pin) {
  case 8:  return 2;
  case 9:  return 3;
  case 7:  return 4;
  case 0:  return 17;
  case 2:  return 27;
  case 3:  return 22;
  case 12: return 10;
  case 13: return 9;
  case 14: return 11;

  case 30: return 0;
  case 21: return 5;
  case 22: return 6;
  case 23: return 13;
  case 24: return 19;
  case 25: return 26;

  case 17: return 28;
  case 19: return 30;

  case 15: return 14;
  case 16: return 15;
  case 1:  return 18;
  case 4:  return 23;
  case 5:  return 24;
  case 6:  return 25;
  case 10: return 8;
  case 11: return 7;

  case 31: return 1;
  case 26: return 12;
  case 27: return 16;
  case 28: return 20;
  case 29: return 21;

  case 18: return 29;
  case 20: return 31;

  default:
    assert("Unknown WiringPI pin");
    return 0;
  }
};

extern "C" {

int wiringPiSetup(const char *port) {
  if (!gpio_is_open()) {
    if (!gpio_open(port)) {
      return 0;
    }
  }
  return 1;
}

void digitalWrite(int pin, int state) {
  pin = wpi_pin(pin);
  gpio_write(pin, state);
}

int digitalRead(int pin) {
  pin = wpi_pin(pin);
  return gpio_read(pin) ? 1 : 0;
}

void pinMode(int pin, int state) {
  pin = wpi_pin(pin);
  if (state == 0) {
    gpio_input(pin);
  }
  if (state == 1) {
    gpio_output(pin);
  }
}

uint64_t millis() {
  static uint64_t ticks = 0;
  if (ticks == 0) {
    ticks = GetTickCount64();
  }
  return GetTickCount64() - ticks;
}

void delay(uint64_t ms) {
#ifdef __cplusplus
  std::this_thread::sleep_for(std::chrono::milliseconds(ms));
#endif
}

void delayMicroseconds(uint64_t us) {
#ifdef __cplusplus
//  std::this_thread::sleep_for(std::chrono::microseconds(us));
#endif
}

}  // extern "C"
