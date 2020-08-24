#ifndef GAME_H
#define GAME_H

#include <am.h>
#include <amdev.h>
#include <klib.h>

#define SIDE 16

static inline void puts(const char *s)
{
    for (; *s; s++)
        _putc(*s);
}

struct Config
{
    int winnie_v;
    int missile_v;
    int missile_f;
    int frog_v;
    int frog_f;
    int honey_v;
    int honey_f;
    int honey_p;
    int score_hit_by_frog;
    int score_kill_frog;
    int score_hit_by_honey;
    int score_kill_honey;
    int score_tick;
    int max_HP;
};
typedef struct Config Config;
extern Config config;

#endif