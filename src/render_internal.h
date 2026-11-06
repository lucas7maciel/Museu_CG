#ifndef MUSEUM_RENDER_INTERNAL_H
#define MUSEUM_RENDER_INTERNAL_H

#include "render.h"
#include "textures.h"

/* Estado de desenho acessivel apenas aos modulos que emitem geometria. */
typedef struct {
    double material_emission,object_fill,geometry_step;
    int collecting_shadows;
    GLuint earth_scene_list;
} RenderState;
extern RenderState renderer;

Vec3 render_color(Vec3 point,Vec3 normal,Vec3 color,double emission);
Vec3 object_world(Vec3 p,int direction);
void object_begin(Vec3 origin,double angle);
void object_end(void);
void object_begin_roll(Vec3 origin,double angle);
void material(Vec3 color, int tex, double shiny);
void box(Vec3 pos, Vec3 size, Vec3 color, int tex, double shiny);
void draw_sphere(Vec3 center,double radius,Vec3 base);
void build_earth_mesh(void);
void draw_earth(void);
void draw_balance(void);
void draw_exhibit_static(void);
void draw_exhibit_dynamic(void);
void collect_light_occluders(void);
void clear_render_cache(void);

void ring_z(double inner,double outer,double depth,Vec3 base);

#endif
