#include "klib.h"

#if !defined(__ISA_NATIVE__) || defined(__NATIVE_USE_KLIB__)

size_t strlen(const char* s) {
    int i = 0;
    while (s[i] != '\0') {
        i++;
    }
    return i;
}

char* strcpy(char* dst, const char* src) {
    size_t i = 0;
    while (src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }
    dst[i] = '\0';
    return dst;
}

char* strncpy(char* dst, const char* src, size_t n) {
    int i = 0;
    while (src[i] != '\0' && i < n) {
        dst[i] = src[i];
        i++;
    }
    for (; i < n; i++) {
        dst[i] = '\0';
    }
    return dst;
}

char* strcat(char* dst, const char* src) {
    size_t start = strlen(dst);
    int i = 0;
    while (src[i] != '\0') {
        dst[start + i] = src[i];
        i++;
    }
    dst[start + i] = '\0';
    return dst;
}

int strcmp(const char* s1, const char* s2) {
    int i = 0, ans = 0;
    while (s1[i] != '\0') {
        ans = s1[i] - s2[i];
        if (ans != 0) {
            return ans;
        }
        i++;
    }
    ans = s1[i] - s2[i];
    return ans;
}

int strncmp(const char* s1, const char* s2, size_t n) {
    int i = 0, ans = 0;
    while (s1[i] != '\0' && i < n) {
        ans = s1[i] - s2[i];
        if (ans != 0) {
            return ans;
        }
        i++;
    }
    if (i < n) {
        ans = s1[i] - s2[i];
        return ans;
    }
    return 0;
}

void* memset(void* v, int c, size_t n) {
    int i;
    for (i = 0; i < n; i++) {
        ((char*)v)[i] = (unsigned char)c;
    }
    return v;
}

void* memcpy(void* out, const void* in, size_t n) {
    // printf("ret:%d\n", *((int*)(&out-4)));
    // printf("out=%d in=%d n=%d\n", out, in, n);
    int i;
    for (i = 0; i < n; i++) {
        ((char*)out)[i] = ((char*)in)[i];
    }
    // printf("memcpy out=%d in=%d size=%d\n", out, in, n);
    return out;
}

int memcmp(const void* s1, const void* s2, size_t n) {
    int i, ans = 0;
    for (i = 0; i < n; i++) {
        ans = ((char*)s1)[i] - ((char*)s2)[i];
        if (ans != 0) {
            return ans;
        }
    }
    return 0;
}

void* memmove(void* dest, const void* src, size_t n) {
    // idea from xv6

    const char* s;
    char* d;

    s = src;
    d = dest;
    if (s < d && s + n > d) {
        s += n;
        d += n;
        while (n-- > 0) {
            *--d = *--s;
        }
    } else {
        while (n-- > 0) {
            *d++ = *s++;
        }
    }

    return dest;
}

#endif
