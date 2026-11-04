#ifndef MUSEUM_RENDER_H
#define MUSEUM_RENDER_H

#include "museum.h"
#include "platform_gl.h"

int render_init(const char *executable);
void render_shutdown(void);
void scene(void);
void crosshair(int w,int h);
int screenshot(const char *path,int w,int h);

#endif
