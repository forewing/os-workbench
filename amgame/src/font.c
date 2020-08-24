#include <game.h>
#include <draw.h>
#include <font.h>

#include <font_data.h>

void draw_char(char c, int x, int y, int color_char, int color_back)
{
    if ((int)c > 127)
    {
        return;
    }
    int pixel_width = char_p * p;
    for (int i = 0; i < char_h; i++)
    {
        for (int j = 0; j < char_w; j++)
        {
            int bit1 = (fonts[(int)c * 2] >> (63 - (i * char_h + j))) & 1;
            draw_rect_pure(x + pixel_width * char_w - j * pixel_width, y + i * pixel_width, pixel_width, pixel_width, bit1 == 1 ? color_char : color_back);
            int bit2 = (fonts[(int)c * 2 + 1] >> (63 - (i * char_h + j))) & 1;
            draw_rect_pure(x + pixel_width * char_w - j * pixel_width, y + i * pixel_width + char_h * pixel_width, pixel_width, pixel_width, bit2 == 1 ? color_char : color_back);
        }
    }
}

void draw_string(const char *str, int x, int y, int color_char, int color_back)
{
    int len = strlen(str);
    draw_rect_pure(x, y, len * char_p * p * (char_w + char_space), char_h * char_p * p * 2, color_back);
    for (int i = 0; i < len; i++)
    {
        draw_char(str[i], x + i * char_p * p * (char_w + char_space), y, color_char, color_back);
    }
}

void draw_string_line(const char *str, int x, int line, int color_char, int color_back)
{
    if (line >= 0)
    {
        draw_string(str, x, ((char_h * 2 * char_p + char_space) * line) * p, color_char, color_back);
    }
    else
    {
        draw_string(str, x, h + ((char_h * 2 * char_p + char_space) * line) * p, color_char, color_back);
    }
}

void draw_string_line_middle(const char *str, int line, int color_char, int color_back)
{
    int len = strlen(str);
    draw_string_line(str, w / 2 - (len * char_p * p * (char_w + char_space) / 2), line, color_char, color_back);
}