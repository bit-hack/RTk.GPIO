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

static serial_t* serial;

extern "C" {

bool gpio_open(const char *port) {
  if (serial) {
    serial_close(serial);
    serial = NULL;
  }
  const int baud = 230400;
  serial = serial_open(port, baud);
  return serial != nullptr;
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
  assert(pin >= 0 && pin <= 27);
  if (serial) {
    char data[] = { 'a' + char(pin), 'I' };
    serial_send(serial, data, sizeof(data));
  }
}

void gpio_output(int pin) {
  assert(pin >= 0 && pin <= 27);
  if (serial) {
    char data[] = { 'a' + char(pin), 'O' };
    serial_send(serial, data, sizeof(data));
  }
}

void gpio_write(int pin, bool state) {
  assert(pin >= 0 && pin <= 27);
  if (serial) {
    char data[] = { 'a' + char(pin), state ? '1' : '0' };
    serial_send(serial, data, sizeof(data));
  }
}

bool gpio_read(int pin) {
  assert(pin >= 0 && pin <= 27);
  char data[4] = { 'a' + char(pin), '?', '\0', '\0' };
  if (serial) {
    serial_send(serial, data, 2);  // example: a?
    serial_read(serial, data, 4);  // example: a0\r\n
    assert(data[2] == '\r');
    assert(data[3] == '\n');
  }
  return (data[1] == '1') ? true : false;
}

void gpio_pull(int pin, int state) {
  assert(pin >= 0 && pin <= 27);
  if (serial) {
    char data[2] = { 'a' + char(pin), (state == 1) ? 'U' :   // up
                                      (state == 0) ? 'D' :   // down
                                                     'N' };  // none
    serial_send(serial, data, sizeof(data));
  }
}

void gpio_board_version(char* dst, uint32_t dst_size) {
  assert(dst && dst_size);
  if (serial) {
    const char* end = dst + (dst_size - 1);
    // request the version string
    // note: we send two bytes but the second is ignored but required by the firmware
    serial_send(serial, "V_", 2);

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

  gpio_output(cs);
  gpio_write (cs,   1);  // cs high (not asserted)
  gpio_output(mosi);
  gpio_write (mosi, 1);
  gpio_output(sck);
  gpio_write (sck,  1);
  gpio_input (miso);
}

uint8_t spi_send(int sck, int mosi, int miso, uint8_t data, int cs) {

  // pull CS low
  if (cs >= 0) gpio_write(cs, 0);

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
  if (cs >= 0) gpio_write(cs, 1);

  return recv;
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
