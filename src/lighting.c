#include "lighting.h"
#include "earth.h"
#include "balance.h"
#include "exhibits.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define MAX_OCCLUDERS 512
#define V(x,y,z) ((Vec3){(x),(y),(z)})
#define WARM_COLOR {1,.83,.61}
#define EXHIBIT_COLOR WARM_COLOR
#define CEILING(x,z) {LIGHT_SPOT,{x,5.35,z},{0,0,0},WARM_COLOR,2.65,9,{0,0,0}}
#define STRIP(x0,z0,x1,z1,nx,nz) {LIGHT_STRIP,{x0,.90,z0},{x1,.90,z1},WARM_COLOR,1.7,3,{nx,0,nz}}
#define EXHIBIT(x,z,dz) {LIGHT_EXHIBIT_SPOT,{x,5.25,z},{0,-1,dz},EXHIBIT_COLOR,1.6,7,{0,0,0}}

const Vec3 museum_warm_color=WARM_COLOR,museum_exhibit_color=EXHIBIT_COLOR;

/* Teto e rodapes compartilham a mesma cor quente. As fitas seguem o contorno
   interno dos saloes e corredor, com interrupcao na entrada e sem cruzar vaos. */
const MuseumLight museum_lights[] = {
    CEILING(SCULPTURE_X,SCULPTURE_Z),CEILING(PENDULUM_X,PENDULUM_Z),
    CEILING(EARTH_X,EARTH_Z),CEILING(BALANCE_X,BALANCE_Z),
    CEILING(-4.0,-14),CEILING(4.5,-9.7),
    STRIP(-7.98,.22,-7.98,19.78,1,0),
    STRIP( 7.98,.22, 7.98,19.78,-1,0),
    STRIP(-7.98,19.78,-2.52,19.78,0,-1),
    STRIP( 2.52,19.78, 7.98,19.78,0,-1),
    STRIP(-7.98,.22,-1.88,.22,0,1),
    STRIP( 1.88,.22, 7.98,.22,0,1),
    STRIP(-1.88,-7.82,-1.88,.22,1,0),
    STRIP( 1.88,-7.82, 1.88,.22,-1,0),
    STRIP(-7.98,-7.82,-1.88,-7.82,0,-1),
    STRIP( 1.88,-7.82, 8.58,-7.82,0,-1),
    STRIP(-7.98,-19.78,-7.98,-7.82,1,0),
    STRIP( 8.58,-19.78, 8.58,-7.82,-1,0),
    STRIP(-7.98,-19.78,8.58,-19.78,0,1),
    /* Fonte centralizada sob a Terra; raios paralelos verticais para cima. */
    {LIGHT_UP_DIRECTIONAL,{EARTH_X,.82,EARTH_Z},{0,1,0},EXHIBIT_COLOR,2.2,5,{0,0,0}},
    /* Refletores discretos para as obras; o palco RGB usa suas cores proprias. */
    EXHIBIT(SCULPTURE_X,SCULPTURE_Z+1.7,-.6),EXHIBIT(PENDULUM_X,PENDULUM_Z+1.6,-.6),
    EXHIBIT(BALANCE_X,BALANCE_Z+1.6,-.6),EXHIBIT(-4.1,-12.8,-.9)
};
const size_t museum_light_count=sizeof(museum_lights)/sizeof(museum_lights[0]);
int ceiling_lights_on=1,corridor_lights_on=1,earth_light_on=1,exhibit_lights_on=1;
static int rgb_enabled[3]={1,1,1};
static double rgb_green_x=4.5;

typedef struct { Vec3 center,half; double c,s,radius; int roll; } Occluder;
static Occluder occluders[MAX_OCCLUDERS];
static size_t occluder_count;
static struct { Vec3 center; double radius; } spheres[32];
static size_t sphere_count;
static LightingStats stats;
LightingStats lighting_stats(void) { return stats; }

static double clamp(double x,double lo,double hi) { return fmax(lo,fmin(hi,x)); }
static Vec3 add(Vec3 a,Vec3 b) { return V(a.x+b.x,a.y+b.y,a.z+b.z); }
static Vec3 sub(Vec3 a,Vec3 b) { return V(a.x-b.x,a.y-b.y,a.z-b.z); }
static Vec3 scale(Vec3 a,double k) { return V(a.x*k,a.y*k,a.z*k); }
static double dot(Vec3 a,Vec3 b) { return a.x*b.x+a.y*b.y+a.z*b.z; }
static Vec3 normalized(Vec3 a) { return scale(a,1/fmax(sqrt(dot(a,a)),1e-9)); }

void lighting_clear_occluders(void) { occluder_count=0; sphere_count=0; }
void lighting_add_sphere(Vec3 center,double radius) {
    if (sphere_count>=sizeof(spheres)/sizeof(spheres[0])) { fputs("Limite de esferas de sombra excedido.\n",stderr); exit(EXIT_FAILURE); }
    spheres[sphere_count].center=center; spheres[sphere_count++].radius=radius;
}
void lighting_add_occluder(Vec3 center,Vec3 size,double rotation) {
    if (occluder_count>=MAX_OCCLUDERS) { fputs("Limite de oclusores de luz excedido.\n",stderr); exit(EXIT_FAILURE); }
    Occluder *o=&occluders[occluder_count++];
    o->center=center; o->half=scale(size,.5);
    o->c=cos(rotation*PI/180); o->s=sin(rotation*PI/180);
    o->radius=sqrt(dot(o->half,o->half));
    o->roll=0;
}

void lighting_add_rolled_occluder(Vec3 center,Vec3 size,double rotation) {
    lighting_add_occluder(center,size,rotation);
    occluders[occluder_count-1].roll=1;
}

static int ray_hits(Vec3 origin,Vec3 direction,double distance,const Occluder *box) {
    Vec3 rel=sub(box->center,origin);
    double projection=dot(rel,direction);
    /* Esfera envolvente elimina a maioria das caixas antes do teste exato. */
    if (projection < -box->radius || projection > distance+box->radius ||
        dot(rel,rel)-projection*projection > box->radius*box->radius) return 0;
    Vec3 p=V(-box->c*rel.x+box->s*rel.z,-rel.y,-box->s*rel.x-box->c*rel.z);
    Vec3 d=V(box->c*direction.x-box->s*direction.z,direction.y,box->s*direction.x+box->c*direction.z);
    if (box->roll) {
        p=V(-box->c*rel.x-box->s*rel.y,box->s*rel.x-box->c*rel.y,-rel.z);
        d=V(box->c*direction.x+box->s*direction.y,-box->s*direction.x+box->c*direction.y,direction.z);
    }
    double mn=.004,mx=distance-.018;
    double origins[]={p.x,p.y,p.z},dirs[]={d.x,d.y,d.z},halves[]={box->half.x,box->half.y,box->half.z};
    for (int i=0;i<3;++i) {
        if (fabs(dirs[i])<1e-9) { if (fabs(origins[i])>halves[i]) return 0; }
        else {
            double a=(-halves[i]-origins[i])/dirs[i],b=(halves[i]-origins[i])/dirs[i];
            mn=fmax(mn,fmin(a,b)); mx=fmin(mx,fmax(a,b));
            if (mn>mx) return 0;
        }
    }
    return mx>=mn;
}

double lighting_visibility(Vec3 point,Vec3 direction,double distance) {
    ++stats.shadow_rays;
    for (size_t i=0;i<sphere_count;++i) {
        double sphere_radius=spheres[i].radius;
        Vec3 p=sub(point,spheres[i].center);
        double b=dot(p,direction),c=dot(p,p)-sphere_radius*sphere_radius,discriminant=b*b-c;
        if (discriminant>=0) {
            double hit=-b-sqrt(discriminant);
            if (hit>.004 && hit<distance-.018) return 0;
            /* Um ponto no interior tambem tem a saida da esfera bloqueada. */
            if (c<-.001 && -b+sqrt(discriminant)>.004) return 0;
        }
    }
    for (size_t i=0;i<occluder_count;++i)
        if (ray_hits(point,direction,distance,&occluders[i])) return 0;
    return 1;
}

Vec3 lighting_sun_direction(void) { return normalized(V(-1,.65,.18)); }

void lighting_set_rgb(const int enabled[3],double green_x) {
    for (int i=0;i<3;++i) rgb_enabled[i]=enabled[i];
    rgb_green_x=green_x;
}
Vec3 lighting_rgb_position(int channel) {
    return V(channel==0 ? 2.8 : channel==1 ? rgb_green_x : 6.4,channel==1 ? RGB_GREEN_Y : 3.65,-12.5);
}

Vec3 lighting_sample(Vec3 point,Vec3 normal,Vec3 color,double emission,int sun_on) {
    ++stats.samples;
    /* Base interior baixa; o exterior recebe uma parcela difusa do ceu. */
    int indoors=point.y>.65 && point.y<5.73 &&
        ((point.x>-8.35 && point.x<8.35 && point.z>-.15 && point.z<20.15) ||
         (point.x>-2.25 && point.x<2.25 && point.z>-7.75 && point.z<.15) ||
         (point.x>-8.35 && point.x<8.95 && point.z>-20.15 && point.z<-7.45));
    double ambient=indoors ? .035 : .20;
    Vec3 light=indoors ? scale(museum_warm_color,ambient) : V(ambient*.85,ambient*.92,ambient);
    Vec3 start=add(point,scale(normal,.018));
    if (sun_on) {
        Vec3 direction=lighting_sun_direction();
        double diffuse=fmax(0,dot(normal,direction));
        if (diffuse>0 && lighting_visibility(start,direction,100))
            light=add(light,scale(V(1,.91,.73),diffuse*1.15));
    }
    for (size_t i=0;i<museum_light_count;++i) {
        const MuseumLight *source=&museum_lights[i];
        if (source->type==LIGHT_UP_DIRECTIONAL) {
            if (!earth_light_on || point.y<=source->position.y || point.y-source->position.y>source->range) continue;
            Vec3 ray=normalized(source->end),direction=scale(ray,-1);
            double distance=(point.y-source->position.y)/ray.y;
            Vec3 footprint=sub(point,scale(ray,distance));
            double radius=hypot(footprint.x-source->position.x,footprint.z-source->position.z);
            double weight=clamp((2.05-radius)/.65,0,1),diffuse=fmax(0,dot(normal,direction));
            if (weight>0 && diffuse>0 && lighting_visibility(start,direction,distance))
                light=add(light,scale(source->color,source->intensity*weight*diffuse));
            continue;
        }
        if ((source->type==LIGHT_SPOT && !ceiling_lights_on) ||
            (source->type==LIGHT_STRIP && !corridor_lights_on) ||
            (source->type==LIGHT_EXHIBIT_SPOT && !exhibit_lights_on)) continue;
        Vec3 source_pos=source->position;
        if (source->type==LIGHT_STRIP) {
            Vec3 line=sub(source->end,source->position);
            source_pos=add(source_pos,scale(line,clamp(dot(sub(point,source_pos),line)/dot(line,line),0,1)));
        }
        Vec3 delta=sub(source_pos,point);
        double distance2=dot(delta,delta);
        if (distance2>=source->range*source->range || distance2<1e-12) continue;
        double distance=sqrt(distance2);
        Vec3 direction=scale(delta,1/distance);
        double diffuse=fmax(0,dot(normal,direction)),cone=1;
        if (diffuse<=0) continue;
        if (source->type==LIGHT_SPOT) {
            /* Cone externo 54 graus, nucleo suave de 31 graus. */
            cone=clamp((direction.y-.588)/(.857-.588),0,1);
            cone=cone*cone*(3-2*cone);
            if (cone<=0) continue;
        } else if (source->type==LIGHT_EXHIBIT_SPOT) {
            double alignment=dot(scale(direction,-1),normalized(source->end));
            cone=clamp((alignment-.78)/(.94-.78),0,1);
            cone=cone*cone*(3-2*cone);
            if (cone<=0) continue;
        }
        double attenuation=source->type==LIGHT_STRIP ? 1/(1+2.5*distance*distance) :
            1/(1+.25*distance+.14*distance*distance);
        double edge=clamp((source->range-distance)/1.2,0,1);
        if (lighting_visibility(start,direction,distance))
            light=add(light,scale(source->color,source->intensity*attenuation*diffuse*cone*edge));
    }
    if (point.z<-8 && point.z>-21 && point.x>-.5) for (int channel=0;channel<3;++channel) {
        if (!rgb_enabled[channel]) continue;
        Vec3 source=lighting_rgb_position(channel),delta=sub(source,point);
        double distance=sqrt(dot(delta,delta));
        if (distance<.001 || distance>8.5) continue;
        Vec3 direction=scale(delta,1/distance),aim=normalized(sub(V(RGB_X,2.6,RGB_SCREEN_Z),source));
        double cone=clamp((dot(scale(direction,-1),aim)-.574)/(.85-.574),0,1);
        double diffuse=fmax(0,dot(normal,direction));
        if (cone<=0 || diffuse<=0 || !lighting_visibility(start,direction,distance)) continue;
        double value=3.8*diffuse*cone*cone*(3-2*cone)/(1+.10*distance+.11*distance*distance);
        if (channel==0) light.x+=value;
        else if (channel==1) light.y+=value;
        else light.z+=value;
    }
    return V(clamp(color.x*(light.x+emission),0,1),clamp(color.y*(light.y+emission),0,1),clamp(color.z*(light.z+emission),0,1));
}

int lighting_self_test(void) {
    lighting_clear_occluders();
    lighting_add_occluder(V(0,2,0),V(.2,4,4),0);
    if (lighting_visibility(V(-2,2,0),V(1,0,0),4)!=0 ||
        lighting_visibility(V(-2,5,0),V(1,0,0),4)!=1 ||
        lighting_visibility(V(-2,2,0),V(1,0,0),1)!=1) return EXIT_FAILURE;
    lighting_clear_occluders();
    lighting_add_occluder(V(0,2,0),V(.2,4,4),90);
    if (lighting_visibility(V(0,2,-2),V(0,0,1),4)!=0) return EXIT_FAILURE;
    lighting_clear_occluders();
    lighting_add_rolled_occluder(V(0,2,0),V(4,.2,1),90);
    if (lighting_visibility(V(-2,2,0),V(1,0,0),4)!=0 ||
        lighting_visibility(V(-2,5,0),V(1,0,0),4)!=1) return EXIT_FAILURE;
    lighting_clear_occluders();
    lighting_add_sphere(V(-2,2,0),.4); lighting_add_sphere(V(2,2,0),.4);
    if (lighting_visibility(V(-2,2,-2),V(0,0,1),4)!=0 ||
        lighting_visibility(V(2,2,-2),V(0,0,1),4)!=0) return EXIT_FAILURE;
    lighting_clear_occluders();
    ceiling_lights_on=0; corridor_lights_on=0; exhibit_lights_on=0;
    Vec3 dark=lighting_sample(V(0,.72,5),V(0,1,0),V(1,1,1),0,0);
    ceiling_lights_on=1;
    Vec3 lit=lighting_sample(V(SCULPTURE_X,.72,SCULPTURE_Z),V(0,1,0),V(1,1,1),0,0);
    if (dark.x>.05 || lit.x<dark.x+.25) return EXIT_FAILURE;
    lighting_add_occluder(V(SCULPTURE_X,3,SCULPTURE_Z),V(2,.2,2),0);
    Vec3 blocked=lighting_sample(V(SCULPTURE_X,.72,SCULPTURE_Z),V(0,1,0),V(1,1,1),0,0);
    if (blocked.x>=lit.x*.5) return EXIT_FAILURE;
    lighting_clear_occluders(); corridor_lights_on=1;
    ceiling_lights_on=0; corridor_lights_on=0; earth_light_on=0;
    int red_only[]={1,0,0},all[]={1,1,1};
    lighting_set_rgb(red_only,4.5);
    Vec3 red=lighting_sample(V(4.5,3,-17.65),V(0,0,1),V(1,1,1),0,0);
    lighting_set_rgb(all,4.5);
    Vec3 white=lighting_sample(V(4.5,3,-17.65),V(0,0,1),V(1,1,1),0,0);
    lighting_set_rgb(all,6.4);
    Vec3 shifted=lighting_sample(V(4.5,3,-17.65),V(0,0,1),V(1,1,1),0,0);
    if (red.x<red.y+.2 || white.y<red.y+.2 || fabs(white.y-shifted.y)<.01) return EXIT_FAILURE;
    lighting_set_rgb(all,4.5); ceiling_lights_on=corridor_lights_on=earth_light_on=1;
    exhibit_lights_on=1;
    Vec3 exhibit_on=lighting_sample(V(SCULPTURE_X,2.5,SCULPTURE_Z+.5),V(0,0,1),V(1,1,1),0,0);
    exhibit_lights_on=0;
    Vec3 exhibit_off=lighting_sample(V(SCULPTURE_X,2.5,SCULPTURE_Z+.5),V(0,0,1),V(1,1,1),0,0);
    if (exhibit_on.z<exhibit_off.z+.08) return EXIT_FAILURE;
    exhibit_lights_on=1;
    puts("OK: luzes, alcance, caixas giradas, oclusao e ambiente escuro.");
    return EXIT_SUCCESS;
}
