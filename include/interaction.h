#ifndef MUSEUM_INTERACTION_H
#define MUSEUM_INTERACTION_H

#include "museum.h"
#include "platform_gl.h"

void update_earth(double dt);
void update_balance(double dt);
int interaction_target(Vec3 origin, Vec3 direction);
int aimed_target(void);
void mouse_button_callback(GLFWwindow *win, int button, int action, int mods);
void focus_callback(GLFWwindow *win,int focused);
void update_exhibits(double dt);
void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods);

#endif
