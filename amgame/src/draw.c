#include <game.h>
#include <draw.h>
#include <model.h>
#include <font.h>

int w, h, p;

void init_screen()
{
    w = screen_width();
    h = screen_height();

    p = w > h ? h : w;
    p /= 200;
    if (p < 1)
    {
        p = 1;
    }

    clear_screen();

    printf("Init with w = %d, h = %d\n", w, h);
}

void splash()
{
    draw_modele_all_clear();
    draw_modele_all();
    draw_sync();
}

void clear_screen()
{
    draw_rect_pure(0, 0, w, h, COLOR_WHITE);
    draw_sync();
}

static void draw_rect_small(int x, int y, int wt, int ht, uint32_t color)
{
    if (x + wt > w || y + ht > h)
    {
        return;
    }
    uint32_t pixels[wt * ht]; // WARNING: allocated on stack
    for (int i = 0; i < wt * ht; i++)
    {
        pixels[i] = color;
    }
    draw_rect(pixels, x, y, wt, ht);
}

void draw_rect_pure(int x, int y, int w, int h, uint32_t color)
{
    for (int ix = 0; ix * SIDE <= w; ix++)
    {
        int dx = w - ix * SIDE;
        if (dx > SIDE)
        {
            dx = SIDE;
        }

        for (int iy = 0; iy * SIDE <= h; iy++)
        {
            int dy = h - iy * SIDE;
            if (dy > SIDE)
            {
                dy = SIDE;
            }
            draw_rect_small(x + ix * SIDE, y + iy * SIDE, dx, dy, color);
        }
    }
}

void draw_HP(int HP)
{
    int pixel_width = p * Heart.p;
    draw_rect_pure(0, h - pixel_width * (Heart.h + 1), pixel_width * ((config.max_HP + 1) * (Heart.w + 1) + 1), (Heart.h + 1) * pixel_width, COLOR_WHITE);
    for (int i = 0; i < HP; i++)
    {
        ModelE e;
        e.model = &Heart;
        e.x = pixel_width * (i * (Heart.w + 1) + 1);
        e.y = h - pixel_width * (Heart.h + 1);
        draw_modele(&e);
    }
}

static char str[128];
void draw_score(int score)
{
    sprintf(str, "Score: %d", score);
    draw_rect_pure(0, 0, w, (char_h * 2 + 2 * char_space) * char_p * p, COLOR_BLUE);
    draw_string(str, char_space * char_p * p, char_space * char_p * p, COLOR_WHITE, COLOR_BLUE);
}

void draw_missile(int stat)
{
    if (stat)
    {
        nuke_on();
    }
    else
    {
        nuke_off();
    }
    int pixel_width = NukeBtn.p * p;
    ModelE e;
    e.model = &NukeBtn;
    e.x = w - pixel_width * (NukeBtn.w + 1);
    e.y = h - pixel_width * (NukeBtn.h + 1);
    draw_modele(&e);
}