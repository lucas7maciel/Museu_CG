#ifndef MUSEUM_CAMERA_H
#define MUSEUM_CAMERA_H

#include "museum.h"
#include "platform_gl.h"

Vec3 look_direction(void);
void move_player(double dt, int forward, int side, int running);
void keyboard_look(double dt,int horizontal,int vertical);
void mouse_callback(GLFWwindow *win, double x, double y);
void camera(int w, int h);

#endif
