#ifndef MUSEUM_BALANCE_H
#define MUSEUM_BALANCE_H

#include "lighting.h"
#include "museum_layout.h"

#define BALANCE_X MAIN_EXHIBIT_RIGHT_X
#define BALANCE_Z MAIN_EXHIBIT_ENTRY_Z
#define BALANCE_PIVOT_Y 3.2
#define BALANCE_ARM 1.75
#define BALANCE_DROP 1.1
#define BALANCE_MIN_RADIUS .30
#define BALANCE_MAX_RADIUS .85
#define BALANCE_INITIAL_RADIUS .45
#define BALANCE_GROW_SPEED .24
#define BALANCE_MAX_TILT 22.0
#define TARGET_BALANCE_LEFT 8
#define TARGET_BALANCE_RIGHT 9

typedef struct {
    double radius[2],angle;
    int growing[2],held;
} BalanceState;

void balance_reset(BalanceState *state);
void balance_update(BalanceState *state,double dt);
double balance_target_angle(const BalanceState *state);
Vec3 balance_endpoint(const BalanceState *state,int side);
Vec3 balance_ball_center(const BalanceState *state,int side);
int balance_self_test(void);

#endif
