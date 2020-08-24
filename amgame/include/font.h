#ifndef FONT_H
#define FONT_H

#define char_p 1
#define char_w 8
#define char_h 8
#define char_space 1

void draw_char(char c, int x, int y, int color_char, int color_back);
void draw_string(const char *str, int x, int y, int color_char, int color_back);
void draw_string_line(const char *str, int x, int line, int color_char, int color_back);
void draw_string_line_middle(const char *str, int line, int color_char, int color_back);

#endif