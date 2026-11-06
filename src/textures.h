#ifndef MUSEUM_TEXTURES_H
#define MUSEUM_TEXTURES_H

#include "museum.h"
#include "platform_gl.h"

enum { T_NONE,T_GRASS,T_STONE,T_EARTH,T_FLOOR,T_WALL,T_CEILING,T_COUNT };

int textures_init(const char *executable);
void textures_shutdown(void);
GLuint texture_handle(int id);
/* Coordenadas em unidades de cena: preservam escala e alinhamento entre blocos. */
void texture_coordinates(int id,Vec3 point,Vec3 normal,double *u,double *v);

#endif
