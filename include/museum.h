#ifndef MUSEUM_STATE_H
#define MUSEUM_STATE_H

#include <math.h>
#include "lighting.h"
#include "earth.h"
#include "balance.h"
#include "exhibits.h"

#define PI 3.14159265358979323846
#define HEIGHT 1.7
#define FLOOR_Y .72
#define WALL_HEIGHT 5.0
#define WALL_T .28
#define ROOF_Y (FLOOR_Y + WALL_HEIGHT)
#define RADIUS .22
#define GATE_Z 20.0
#define GATE_HALF 2.25
#define GATE_WIDTH 2.23
#define GATE_HEIGHT 3.32
#define GATE_T .12
#define GATE_REACH 6.0
#define WINDOW_COUNT 6
#define WINDOW_T .10
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
#define V(x,y,z) ((Vec3){(x),(y),(z)})

typedef struct { double angle; int open, blocked; } WindowState;

/* Estado unico da sessao, compartilhado por controles, cena e testes.
   Dados auxiliares e recursos graficos permanecem privados de seus modulos. */
typedef struct {
    Vec3 position;
    double yaw,pitch,mouse_x,mouse_y;
    int mouse_valid,sunlight_on,unlit_mode;
    double gate_angle,earth_angle;
    int gate_open,gate_blocked,earth_held;
    WindowState windows[WINDOW_COUNT];
    BalanceState balance;
    ExhibitsState exhibits;
} MuseumState;

extern MuseumState museum;

static inline double clamp(double value,double lo,double hi) {
    return fmax(lo,fmin(hi,value));
}

#endif
