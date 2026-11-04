#ifndef MUSEUM_LIGHTING_H
#define MUSEUM_LIGHTING_H

#include <stddef.h>

typedef struct { double x,y,z; } Vec3;
typedef struct { size_t samples,shadow_rays; } LightingStats;
typedef enum { LIGHT_SPOT, LIGHT_STRIP, LIGHT_UP_DIRECTIONAL, LIGHT_EXHIBIT_SPOT } LightType;
typedef struct {
    LightType type;
    Vec3 position, end, color;
    double intensity, range;
    Vec3 mounting_normal;
} MuseumLight;

/* Acrescente fontes nesta tabela em lighting.c; todas usam a mesma oclusao. */
extern const MuseumLight museum_lights[];
extern const size_t museum_light_count;
extern int ceiling_lights_on, corridor_lights_on, earth_light_on, exhibit_lights_on;
extern const Vec3 museum_warm_color,museum_exhibit_color;

void lighting_clear_occluders(void);
void lighting_add_occluder(Vec3 center,Vec3 size,double rotation);
void lighting_add_rolled_occluder(Vec3 center,Vec3 size,double rotation);
void lighting_add_sphere(Vec3 center,double radius);
double lighting_visibility(Vec3 point,Vec3 direction,double distance);
Vec3 lighting_sun_direction(void);
void lighting_set_rgb(const int enabled[3],double green_x);
Vec3 lighting_rgb_position(int channel);
Vec3 lighting_sample(Vec3 point,Vec3 normal,Vec3 color,double emission,int sun_on);
int lighting_self_test(void);
LightingStats lighting_stats(void);

#endif
