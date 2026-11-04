#ifndef MUSEUM_EXHIBITS_H
#define MUSEUM_EXHIBITS_H

#include "lighting.h"
#include "museum_layout.h"

enum {
    TARGET_SCULPTURE=10,TARGET_PENDULUM,
    TARGET_GEAR_0,TARGET_GEAR_1,TARGET_GEAR_2,
    TARGET_RGB_R,TARGET_RGB_G,TARGET_RGB_B,TARGET_RGB_LAMP
};

#define SCULPTURE_X MAIN_EXHIBIT_LEFT_X
#define SCULPTURE_Y 2.35
#define SCULPTURE_Z MAIN_EXHIBIT_CORRIDOR_Z
#define PENDULUM_X MAIN_EXHIBIT_RIGHT_X
#define PENDULUM_Y 4.8
#define PENDULUM_Z MAIN_EXHIBIT_CORRIDOR_Z
#define PENDULUM_LENGTH 2.6
#define PENDULUM_RADIUS .38
#define GEAR_Z (-14.8)
#define RGB_X 4.5
#define RGB_SCREEN_Z (-17.8)
#define RGB_GREEN_Y 4.05

typedef struct {
    double sculpture_open,pendulum_angle,pendulum_velocity,gear_phase,green_x;
    int green_direction,rgb[3],held,pendulum_manual;
    unsigned revision;
} ExhibitsState;

void exhibits_reset(ExhibitsState *s);
void exhibits_press(ExhibitsState *s,int target);
void exhibits_release(ExhibitsState *s);
void exhibits_drag(ExhibitsState *s,double horizontal);
void exhibits_update(ExhibitsState *s,double dt);
Vec3 sculpture_piece(const ExhibitsState *s,int index);
Vec3 pendulum_bob(const ExhibitsState *s);
Vec3 gear_center(int index);
double gear_radius(int index);
int gear_teeth(int index);
double gear_angle(const ExhibitsState *s,int index);
Vec3 rgb_button(int channel);
Vec3 rgb_lamp(const ExhibitsState *s,int channel);
int exhibits_collision(double x,double z);
int exhibits_self_test(void);

#endif
