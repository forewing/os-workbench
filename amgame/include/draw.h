#ifndef DRAW_H
#define DRAW_H

#include <game.h>
#include <model.h>

#define COLOR_BLACK  0x000000
#define COLOR_WHITE  0xFFFFFF
#define COLOR_RED    0xFF0000
#define COLOR_GREEN  0x00FF00
#define COLOR_BLUE   0x0000FF
#define COLOR_YELLO  0xFFFF00
#define COLOR_CYAN   0x00FFFF
#define COLOR_PINK   0xFF00FF
#define COLOR_SILVER 0xC0C0C0
#define COLOR_GRAY   0x808080
#define COLOR_PURPLE 0x800080
#define COLOR_ORANGE 0xF8B932
#define COLOR_JUNGLE 0x143622
#define COLOR_DGREEN 0x009900
#define COLOR_SGREEN 0x9dc305
#define COLOR_BROWN 0x5d4319
#define COLOR_HONEY 0xeaac1e
// #define COLOR_TRANS 0x1000000
#define COLOR_TRANS  COLOR_WHITE
#define COLOR_BACK   COLOR_WHITE

void init_screen();
void clear_screen();
void splash();

void draw_rect_pure(int x, int y, int w, int h, uint32_t color);
void draw_HP(int HP);
void draw_score(int score);
void draw_missile(int stat);

extern int w, h, p;


#endif