#include <game.h>
#include <draw.h>
#include <key.h>
#include <font.h>

#include <model.h>

void init_menu();
int main_loop();
void score_menu(int ret);
void diff_adj(int score);

void list_insert(ModelE *list[], ModelE *modele);
void list_kill(ModelE *list[], ModelE *modele);
void list_kill_all(ModelE *list[]);
void list_clean(ModelE *list[]);
void move_model(ModelE *modele, int up, int down, int left, int right);
int fire_missile(ModelE *firer, Model *missile, int v);
int chk_crash(ModelE *a, ModelE *b);

#define TICK_RATE 24
static int tick_nr = 0;
static int tick()
{
    static uint32_t time_last = 0;
    uint32_t time_now = uptime();
    if ((time_now - time_last) * TICK_RATE >= 1000 || time_now < time_last)
    {
        tick_nr += 1;
        time_last = time_now;
        return 1;
    }
    else
    {
        return 0;
    }
}

int main()
{
    // Operating system is a C program
    _ioe_init();
    init_screen();

    while (1)
    {
        init_menu();
        score_menu(main_loop());
    }

    return 0;
}

Config config;
void init_menu()
{
    clear_screen();

    draw_string_line_middle("WINNIE", 3, COLOR_BLACK, COLOR_ORANGE);
    draw_string_line_middle("love", 4, COLOR_WHITE, COLOR_RED);
    draw_string_line_middle("HONEY", 5, COLOR_BLACK, COLOR_ORANGE);

    draw_string_line("hit RETURN to continue.", 0, -1, COLOR_GRAY, COLOR_WHITE);

    winnie_smile();
    ModelE *winnieE_i = model_spawn(&Winnie, w / 2 - Winnie.p * Winnie.w * p / 2, h / 5 * 3 - Winnie.p * Winnie.h * p, OVF_STOP);
    draw_modele(winnieE_i);

    key_catch_full(_KEY_RETURN);

    model_kill(winnieE_i);

    clear_screen();

    draw_string_line("Press RETURN to start the game.", 0, -3, COLOR_BLACK, COLOR_WHITE);
    draw_string_line("UP, DOWN, LEFT, RIGHT to move,", 0, -2, COLOR_BLACK, COLOR_WHITE);
    draw_string_line("and SPACE to fire.", 0, -1, COLOR_BLACK, COLOR_WHITE);
    printf("Press RETURN to start the game.\n");


    ModelE *honeyE_i = model_spawn(&Honey, w / 2 - Honey.p * Honey.w * p / 2, h / 2 - Honey.p * Honey.h * p, OVF_STOP);
    draw_modele(honeyE_i);

    key_catch_full(_KEY_RETURN);

    model_kill(honeyE_i);

    config.winnie_v = h / TICK_RATE / 3;
    config.missile_v = h / TICK_RATE / 2;
    config.missile_f = (TICK_RATE * 4) / 3;
    config.frog_v = h / TICK_RATE / 2;
    config.frog_f = (TICK_RATE * 2) / 5;
    config.honey_v = h / TICK_RATE / 4;
    config.honey_f = (TICK_RATE * 3) / 2;
    config.honey_p = 2;
    config.score_hit_by_frog = 0;
    config.score_kill_frog = 50;
    config.score_hit_by_honey = 500;
    config.score_kill_honey = -1000;
    config.score_tick = 10;
    config.max_HP = 4;
}

void score_menu(int ret)
{
    if (ret != 0)
    {
        clear_screen();
        char str[64];
        sprintf(str, "YOU DIED with score %d.", ret);
        draw_string_line_middle(str, 5, COLOR_BLACK, COLOR_WHITE);

        winnie_cry();
        ModelE *winnieE_i = model_spawn(&Winnie, w / 2 - Winnie.p * Winnie.w * p / 2, h / 5 * 3 - Winnie.p * Winnie.h * p, OVF_STOP);
        draw_modele(winnieE_i);

        key_catch_full(_KEY_RETURN);

        model_kill(winnieE_i);
    }
    clear_screen();

    draw_string_line_middle("Hit RETURN to start again.", 5, COLOR_BLACK, COLOR_WHITE);

    ModelE *frogE_i = model_spawn(&Frog, w / 2 - Frog.p * Frog.w * p / 2, h / 5 * 3 - Frog.p * Frog.h * p, OVF_STOP);
    draw_modele(frogE_i);

    key_catch_full(_KEY_RETURN);

    model_kill(frogE_i);
}

static ModelE *winnieE;
#define LIST_NR 16
static ModelE *missile_list[LIST_NR];
static ModelE *frog_list[LIST_NR];
static ModelE *honey_list[LIST_NR];

int main_loop()
{
    int score = 0;
    int HP = config.max_HP;

    clear_screen();
    printf("Game started.\n");

    winnie_nmsl();
    winnieE = model_spawn(&Winnie, w / 2 - Winnie.p * Winnie.w * p / 2, h - Winnie.p * Winnie.h * p, OVF_STOP);
    assert(winnieE != NULL);

    while (1)
    {
        read_key_game();
        if (tick())
        {
            winnie_update_auto();
            if (tick_nr % config.score_tick == 0)
            {
                score += 1;
            }
            if (tick_nr % config.frog_f == 0)
            {
                ModelE *frogE = model_spawn(&Frog, (rand() % (w / 2)) - (w / 4) + winnieE->x, 0, OVF_KILL);
                model_setv(frogE, 0, config.frog_v);
                list_insert(frog_list, frogE);
            }

            if (tick_nr % config.honey_f == 0)
            {
                if (rand() % config.honey_p == 0)
                {
                    ModelE *honeyE = model_spawn(&Honey, rand() % w, 0, OVF_KILL);
                    model_setv(honeyE, 0, config.honey_v);
                    list_insert(honey_list, honeyE);
                }
            }

            move_model(winnieE, config.winnie_v, config.winnie_v, config.winnie_v, config.winnie_v);
            int missile_stat = fire_missile(winnieE, &Missile, -config.missile_v);
            model_move_all();

            for (int i = 0; i < LIST_NR; i++)
            {
                for (int j = 0; j < LIST_NR; j++)
                {
                    if (chk_crash(frog_list[j], missile_list[i]))
                    {
                        // printf("Crash frog %d, miss %d\n", i, j);
                        list_kill(frog_list, frog_list[j]);
                        list_kill(missile_list, missile_list[i]);
                        score += config.score_kill_frog;
                        winnie_smile();
                    }
                    if (chk_crash(honey_list[j], missile_list[i]))
                    {
                        // printf("Crash frog %d, miss %d\n", i, j);
                        list_kill(honey_list, honey_list[j]);
                        list_kill(missile_list, missile_list[i]);
                        score += config.score_kill_honey;
                        winnie_cry();
                    }
                }
            }

            for (int i = 0; i < LIST_NR; i++)
            {
                if (chk_crash(winnieE, honey_list[i]))
                {
                    score += config.score_hit_by_honey;
                    list_kill(honey_list, honey_list[i]);
                    HP += 1;
                    if (HP > config.max_HP)
                    {
                        HP = config.max_HP;
                    }
                    winnie_smile();
                }
            }

            for (int i = 0; i < LIST_NR; i++)
            {
                if (chk_crash(winnieE, frog_list[i]))
                {
                    list_kill(frog_list, frog_list[i]);
                    HP -= 1;
                    winnie_cry();
                    // printf("kill frog\n");
                }
            }

            if (HP <= 0)
            {
                break;
            }

            draw_HP(HP);
            draw_score(score);
            draw_missile(missile_stat);

            list_clean(frog_list);
            list_clean(missile_list);
            list_clean(honey_list);

            splash();

            diff_adj(score);
        }
    }

    // model_kill();
    model_kill_all();
    list_kill_all(frog_list);
    list_kill_all(missile_list);
    list_kill_all(honey_list);

    if (HP <= 0)
    {
        return score;
    }
    else
    {
        return 0;
    }
}

void diff_adj(int score)
{
    if (score > 4000)
    {
        config.frog_v = (h / TICK_RATE * 2) / 2;
        config.frog_f = (TICK_RATE * 1) / 5;
    }
    else if (score > 1300)
    {
        config.frog_v = (h / TICK_RATE * 2) / 3;
        config.frog_f = (TICK_RATE * 2) / 7;
    }
}

void list_insert(ModelE *list[], ModelE *modele)
{
    if (modele == NULL)
    {
        printf("Insert fail\n");
    }
    for (int i = 0; i < LIST_NR; i++)
    {
        if (list[i] == NULL || list[i]->valid == 0)
        {
            // printf("insert %d %s\n", i, list == frog_list ? "frog" : "miss");
            list[i] = modele;
            return;
        }
    }
    return;
}

void list_clean(ModelE *list[])
{
    for (int i = 0; i < LIST_NR; i++)
    {
        if (list[i] == NULL)
        {
            continue;
        }
        if (list[i]->valid == 0)
        {
            list[i] = NULL;
        }
    }
}

void list_kill(ModelE *list[], ModelE *modele)
{
    for (int i = 0; i < LIST_NR; i++)
    {
        if (list[i] == modele)
        {
            list[i] = NULL;
            model_kill(modele);
        }
    }
}

void list_kill_all(ModelE *list[])
{
    for (int i = 0; i < LIST_NR; i++)
    {
        list[i] = NULL;
    }
}

void move_model(ModelE *modele, int up, int down, int left, int right)
{
    if (key_chk(_KEY_UP))
    {
        model_move_force(modele, 0, -up);
    }
    if (key_chk(_KEY_DOWN))
    {
        model_move_force(modele, 0, down);
    }
    if (key_chk(_KEY_LEFT))
    {
        model_move_force(modele, -left, 0);
    }
    if (key_chk(_KEY_RIGHT))
    {
        model_move_force(modele, right, 0);
    }
}

int fire_missile(ModelE *firer, Model *missile, int v)
{
    static int tick_last = 0;
    if (tick_nr - tick_last < config.missile_f)
    {
        return 0;
    }
    if (key_chk(_KEY_SPACE))
    {
        tick_last = tick_nr;
        int firer_pixel = firer->model->p * p;
        int missile_pixel = missile->p * p;
        ModelE *missileE = model_spawn(missile, firer->x + firer->model->w * firer_pixel / 2 - missile->w * missile_pixel / 2, firer->y - missile->h * missile_pixel, OVF_KILL);
        model_setv(missileE, 0, v);
        list_insert(missile_list, missileE);
        return 0;
    }
    return 1;
}

int chk_crash(ModelE *a, ModelE *b)
{
    if (a == NULL || b == NULL)
    {
        return 0;
    }
    if (a->valid == 0 || b->valid == 0)
    {
        return 0;
    }
    int ax1 = a->x, ay1 = a->y, ax2 = a->x + a->model->w * a->model->p * p, ay2 = a->y + a->model->h * a->model->p * p;
    int bx1 = b->x, by1 = b->y, bx2 = b->x + b->model->w * b->model->p * p, by2 = b->y + b->model->h * b->model->p * p;
    if (ay2 <= by1 || ay1 >= by2 || ax2 <= bx1 || ax1 >= bx2)
    {
        // printf("No\n");
        return 0;
    }
    else
    {
        // printf("(%d,%d)-(%d,%d) * (%d,%d)-(%d,%d)\n", ax1, ay1, ax2, ay2, bx1, by1, bx2, by2);
        return 1;
    }
}
