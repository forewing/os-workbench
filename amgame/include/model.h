#ifndef MODEL_H
#define MODEL_H

#include <game.h>

// typedef struct Model
// {
//     int w, h;
//     uint32_t* color;
// }Model;
// struct Model;
struct Model
{
    int w, h;
    int p;
    uint32_t *color;
};
typedef struct Model Model;

enum
{
    OVF_KILL,
    OVF_TP,
    OVF_STOP
};
struct ModelE
{
    Model *model;
    int xt, yt;
    int x, y;
    int vx, vy;
    int valid;
    int overflow;
};
typedef struct ModelE ModelE;

void draw_modele(ModelE *modele);
void draw_modele_all();
void draw_modele_clear(ModelE *modele);
void draw_modele_all_clear();

ModelE *model_spawn(Model *model, int x, int y, int ovf);
void model_kill(ModelE *modele);
void model_kill_all();
void model_tp(ModelE *modele, int x, int y);
void model_move_force(ModelE *modele, int dx, int dy);
void model_move_all();
void model_move(ModelE *modele);
void model_setv(ModelE *modele, int vx, int vy);
void model_accel(ModelE *modele, int dvx, int dvy);

extern Model Winnie;
extern Model Missile;
extern Model Frog;
extern Model Heart;
extern Model Honey;
extern Model NukeBtn;

enum
{
    WINNIE_HAPPY = 0,
    WINNIE_NMSL = 1,
    WINNIE_CRY = 2
};

void winnie_smile();
void winnie_nmsl();
void winnie_cry();
void winnie_update_auto();

void nuke_on();
void nuke_off();

#endif