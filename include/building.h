#ifndef MUSEUM_BUILDING_H
#define MUSEUM_BUILDING_H

#include "museum.h"

typedef struct { double x0,x1,z0,z1; } Rect;
typedef struct { Vec3 center,size; } Solid;
typedef struct { double start,end,sill,head; } Opening;

Vec3 gate_local(Vec3 p, int side, double angle, int direction);
int gate_collision(double x, double z, double feet, double height, double angle);
void update_gate(double dt);
void draw_gate(void);
Opening window_opening(int index);
double window_width(int index);
Vec3 window_hinge(int index,int leaf);
Vec3 window_local(Vec3 p,int index,int leaf,double angle,int direction);
Vec3 window_world(Vec3 p,int index,int leaf);
int window_collision(int index,double x,double z,double feet,double height,double angle);
void update_windows(double dt);
void draw_windows(void);
void build_museum(void);
double ground_height(double x, double z);
int wall_collision(double x, double z, double feet, double height);
int walkable(double x, double z, double from_height);
void static_geometry(void);
/* Consulta somente leitura; a tabela pertence ao modulo do edificio. */
const Solid *building_walls(size_t *count);

#endif
