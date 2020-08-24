#include <common.h>
#include <klib.h>
#include <kmt.h>
#include <util.h>

static spinlock_t util_spinlock;
void util_init() {
    kmt->spin_init(&util_spinlock, "util");
}

#ifdef UTIL_LOG_ON

static void log_print(const char* str) {
    const char* ptr = str;
    while (*ptr != '\0') {
        _putc(*ptr);
        ptr += 1;
    }
};

static char num_str[16] = "";
static void _util_log(const char* str,
                      uint32_t num,
                      enum LOG_TYPE type,
                      enum LOG_NMODE mode) {
    switch (type) {
        case LOG_SUCCESS:
            log_print("\033[32m[succ] ");
            break;
        case LOG_INFO:
            log_print("\033[37m[info] ");
            break;
        case LOG_WARNING:
            log_print("\033[33m[warn] ");
            break;
        case LOG_ERROR:
            log_print("\033[31m[erro] ");
            break;
    }

    _putc('#');
    _putc("0123456789"[_cpu()]);
    _putc(' ');

    if (mode != LOG_NNONE) {
        for (int i = 0; i < 8; i++) {
            int mod = num % 16;
            if (mod >= 10) {
                mod += ('a' - 10);
            } else {
                mod += '0';
            }
            num_str[9 - i] = mod;
            num /= 16;
        }
        num_str[0] = '0';
        num_str[1] = 'x';
        num_str[10] = '\0';
        log_print(num_str);
        _putc(' ');
    }

    log_print(str);

    log_print("\033[0m\n");
}
#endif

void util_log(const char* str,
              uint32_t num,
              enum LOG_TYPE type,
              enum LOG_NMODE mode) {
#ifdef UTIL_LOG_ON
    kmt->spin_lock(&util_spinlock);
    _util_log(str, num, type, mode);
    kmt->spin_unlock(&util_spinlock);
#endif
}

uint32_t util_s2u32(const char* str, int len) {
    if (len == 0) {
        len = strlen(str);
    }
    uint32_t tmp = 0;
    int valid = 1;
    for (int i = 0; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') {
            valid = 0;
            break;
        }
        tmp *= 10;
        tmp += str[i] - '0';
    }
    return valid ? tmp : (uint32_t)-1;
}