#ifndef __UTIL_H__
#define __UTIL_H__

#include <common.h>

#define UTIL_LOG_ON

enum LOG_TYPE { LOG_SUCCESS, LOG_INFO, LOG_WARNING, LOG_ERROR };
enum LOG_NMODE { LOG_NNONE, LOG_NDEC, LOG_NHEX };

void util_init();

void util_log(const char* str,
              uint32_t num,
              enum LOG_TYPE type,
              enum LOG_NMODE mode);

uint32_t util_s2u32(const char* str, int len);

#endif