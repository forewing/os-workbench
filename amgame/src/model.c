#include <model.h>
#include <draw.h>

#define MODELE_NR 48
static ModelE modele_list[MODELE_NR];

static int chk_exist(ModelE *modele)
{
    if (modele == NULL)
    {
        // assert(0);
        printf("Error, modele does not exist.\n");
        return 1;
    }
    else
    {
        return 0;
    }
}

ModelE *model_spawn(Model *model, int x, int y, int ovf)
{
    for (int i = 0; i < MODELE_NR; i++)
    {
        if (modele_list[i].valid == 0)
        {
            // printf("Spawned %d\n", i);
            modele_list[i].model = model;
            modele_list[i].valid = 1;
            modele_list[i].x = modele_list[i].xt = x;
            modele_list[i].y = modele_list[i].yt = y;
            modele_list[i].overflow = ovf;
            return &modele_list[i];
        }
    }
    printf("Spawn fail.\n");
    return NULL;
}

void model_kill(ModelE *modele)
{
    if (chk_exist(modele))
    {
        printf("E:kill\n");
        return;
    }
    // printf("Kill\n");
    draw_modele_clear(modele);
    modele->valid = 0;
}

void model_kill_all()
{
    for (int i = 0; i < MODELE_NR; i++)
    {
        modele_list[i].valid = 0;
    }
    clear_screen();
}

void model_tp(ModelE *modele, int x, int y)
{
    if (chk_exist(modele))
    {
        return;
    }
    modele->x = x;
    modele->y = y;
}
void model_move_force(ModelE *modele, int dx, int dy)
{
    if (chk_exist(modele))
    {
        return;
    }
    modele->x += dx;
    modele->y += dy;
}
static void model_ovf(ModelE *modele)
{
    if (chk_exist(modele))
    {
        return;
    }
    switch (modele->overflow)
    {
    case OVF_KILL:
        model_kill(modele);
        break;
    case OVF_TP:
        modele->x %= w;
        modele->y %= h;
        break;
    case OVF_STOP:
        model_tp(modele, modele->xt, modele->yt);
        break;
    default:
        break;
    }
}
void model_move(ModelE *modele)
{
    if (chk_exist(modele))
    {
        return;
    }
    model_move_force(modele, modele->vx, modele->vy);
    int pixel_width = modele->model->p * p;
    // if (modele->x + pixel_width * modele->model->w < 0 || modele->y + pixel_width * modele->model->h < 0 || modele->x > w || modele->y > h)
    if (modele->x < 0 || modele->y < 0 || modele->x + pixel_width * modele->model->w > w || modele->y + pixel_width * modele->model->h > h)
    {
        model_ovf(modele);
    }
}
void model_move_all()
{
    for (int i = 0; i < MODELE_NR; i++)
    {
        if (modele_list[i].valid)
        {
            model_move(&modele_list[i]);
        }
    }
}

void model_setv(ModelE *modele, int vx, int vy)
{
    if (chk_exist(modele))
    {
        return;
    }
    modele->vx = vx;
    modele->vy = vy;
}
void model_accel(ModelE *modele, int dvx, int dvy)
{
    if (chk_exist(modele))
    {
        return;
    }
    modele->vx += dvx;
    modele->vy += dvy;
}

void draw_modele(ModelE *modele)
{
    if (chk_exist(modele))
    {
        // printf("Spawn fail\n");
        return;
    }
    int x0 = modele->x;
    int y0 = modele->y;
    int pixel_width = modele->model->p * p;
    for (int i = 0; i < modele->model->w * modele->model->h; i++)
    {
        draw_rect_pure(x0 + (i % modele->model->w) * pixel_width, y0 + (i / modele->model->w) * pixel_width, pixel_width, pixel_width, modele->model->color[i]);
    }
    // draw_rect_pure();
    // draw_rect(modele->model->color, modele->x, modele->y, modele->model->w, modele->model->h);
    draw_sync();
    modele->xt = modele->x;
    modele->yt = modele->y;
}

void draw_modele_all()
{
    for (int i = 0; i < MODELE_NR; i++)
    {
        if (modele_list[i].valid)
        {
            draw_modele(&modele_list[i]);
        }
    }
}

void draw_modele_clear(ModelE *modele)
{
    if (chk_exist(modele))
    {
        return;
    }
    // if (modele->xt != modele->x || modele->yt != modele->y)
    {
        int pixel_width = modele->model->p * p;
        draw_rect_pure(modele->xt, modele->yt, modele->model->w * pixel_width, modele->model->h * pixel_width, COLOR_BACK);
    }
}

void draw_modele_all_clear()
{
    for (int i = 0; i < MODELE_NR; i++)
    {
        if (modele_list[i].valid)
        {
            draw_modele_clear(&modele_list[i]);
        }
    }
}

#include <model_data.h>

#define WINNIE_TICK 10
static int winnie_tick = 0;
static int winnie_stat = WINNIE_HAPPY;
void winnie_update_auto()
{
    if (winnie_tick > WINNIE_TICK)
    {
        if (winnie_stat == WINNIE_NMSL)
        {
            winnie_smile();
        }
        else
        {
            winnie_nmsl();
        }
    }
    else
    {
        winnie_tick += 1;
    }
}

void winnie_smile()
{
    winnie_tick = 0;
    Winnie.color = winnie_color_smile;
    winnie_stat = WINNIE_HAPPY;
}

void winnie_nmsl()
{
    winnie_tick = 0;
    Winnie.color = winnie_color_nmsl;
    winnie_stat = WINNIE_NMSL;
}

void winnie_cry()
{
    winnie_tick = 0;
    Winnie.color = winnie_color_sad;
    winnie_stat = WINNIE_CRY;
}

void nuke_on()
{
    NukeBtn.color = nukeBtn_color_on;
}

void nuke_off()
{
    NukeBtn.color = nukeBtn_color_off;
}