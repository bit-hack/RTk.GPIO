#pragma once

#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#define wiringPiSPISetup(channel, speed) \
  assert(!"wiringPiSPISetup not supported")

#define wiringPiSPIDataRW(channel, data, len) \
  assert(!"wiringPiSPIDataRW not supported")

#ifdef __cplusplus
}  // extern "C"
#endif
