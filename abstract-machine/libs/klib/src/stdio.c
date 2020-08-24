#include "klib.h"
#include <stdarg.h>

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

static int printf_base(char *out, const char *fmt, va_list valist) {
  out[0] = '\0';

  enum{
    IS_FMT,
    IS_H, IS_HH,
    IS_L, IS_LL,
    IS_POINT,
    IS_LAST
  };
  char flags[IS_LAST];
  char buf_1[8];
  char buf[256];
  memset(flags, 0, IS_LAST);
  memset(buf_1, 0, sizeof(buf_1));
  memset(buf, 0, sizeof(buf));

  int i = 0;
  while(fmt[i] != '\0'){
    switch(fmt[i]){
      case '%':
        if (flags[IS_FMT]){
          strcat(out, "%");
          flags[IS_FMT] = 0;
        }else{
          flags[IS_FMT] = 1;
        }
        break;
      case 'x':
        if (flags[IS_FMT]){
          memset(buf_1, 0, sizeof(buf_1));
          memset(buf, 0, sizeof(buf));
          int n = va_arg(valist, int);
          if (n < 0){
            buf_1[0] = '-';
            n = -n; // error when n = -maxint - 1
          }else if (n == 0){
            buf_1[0] = '0';
          }else{
            buf_1[0] = '\0';
          }
          // TODO()
          volatile int dig = 0; // fuck O2
          while(n != 0){
            buf[dig] = n % 16 + '0';
            if (buf[dig] > '9'){
              buf[dig] += ('a' - '9' - 1);
            }
            n = n / 16;
            dig++;
          }
          int j = 0;
          char FUCKGCC[1];
          // memset(FUCKGCC, dig/2, 0); // fuck O2
          for (j = 0; j < dig / 2; ++j){
            FUCKGCC[0] = buf[j];
            buf[j] = buf[dig - j - 1];
            buf[dig - j - 1] = FUCKGCC[0];
          }
          buf_1[1] = '\0';
          strcat(out, buf_1);
          strcat(out, buf);
          flags[IS_FMT] = 0;
        }else{
          strcat(out, "x");
        }
        break;
      case 'd':
        if (flags[IS_FMT]){
          memset(buf_1, 0, sizeof(buf_1));
          memset(buf, 0, sizeof(buf));
          int n = va_arg(valist, int);
          if (n < 0){
            buf_1[0] = '-';
            n = -n; // error when n = -maxint - 1
          }else if (n == 0){
            buf_1[0] = '0';
          }else{
            buf_1[0] = '\0';
          }
          // TODO()
          volatile int dig = 0; // fuck O2
          while(n != 0){
            buf[dig] = n % 10 + '0';
            n = n / 10;
            dig++;
          }
          int j = 0;
          char FUCKGCC[1];
          // memset(FUCKGCC, dig/2, 0); // fuck O2
          for (j = 0; j < dig / 2; ++j){
            FUCKGCC[0] = buf[j];
            buf[j] = buf[dig - j - 1];
            buf[dig - j - 1] = FUCKGCC[0];
          }
          buf_1[1] = '\0';
          strcat(out, buf_1);
          strcat(out, buf);
          flags[IS_FMT] = 0;
        }else{
          strcat(out, "d");
        }
        break;
      case 's':
        if (flags[IS_FMT]){
          strcat(out, va_arg(valist, char*));
          flags[IS_FMT] = 0;
        }else{
          strcat(out, "s");
        }
        break;
      default:
        buf_1[0] = fmt[i];
        strcat(out, buf_1);
    }
    i++;
  }
  return strlen(out);
}

int printf(const char *fmt, ...) {
  char buf[1024];

  va_list valist;
  va_start(valist, fmt);
  int ret = printf_base(buf, fmt, valist);
  va_end(valist);

  char *ptr;
  for (ptr = buf; *ptr; ptr++){
    _putc(*ptr);
  }

  return ret;
}

int vsprintf(char *out, const char *fmt, va_list ap) {
  return 0;
}

int sprintf(char *out, const char *fmt, ...) {

  va_list valist;
  va_start(valist, fmt);
  int ret = printf_base(out, fmt, valist);
  va_end(valist);

  return ret;
}

int snprintf(char *out, size_t n, const char *fmt, ...) {
  return 0;
}

#endif
