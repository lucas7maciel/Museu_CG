#include "exhibits.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846
#define V(x,y,z) ((Vec3){(x),(y),(z)})
static double clamp(double x,double a,double b) { return fmax(a,fmin(b,x)); }

void exhibits_reset(ExhibitsState *s) {
    unsigned revision=s->revision+1;
    *s=(ExhibitsState){0};
    s->held=-1; s->green_x=4.5; s->green_direction=1;
    s->rgb[0]=s->rgb[1]=s->rgb[2]=1; s->revision=revision;
}
void exhibits_press(ExhibitsState *s,int target) {
    s->held=-1;
    if (target>=TARGET_RGB_R && target<=TARGET_RGB_B) {
        s->rgb[target-TARGET_RGB_R]=!s->rgb[target-TARGET_RGB_R]; ++s->revision;
    } else if (target>=TARGET_SCULPTURE && target<=TARGET_RGB_LAMP) {
        s->held=target;
        if (target==TARGET_PENDULUM) { s->pendulum_velocity=0; s->pendulum_manual=0; }
    }
}
void exhibits_release(ExhibitsState *s) { s->held=-1; }
void exhibits_drag(ExhibitsState *s,double horizontal) {
    if (s->held==TARGET_PENDULUM && fabs(horizontal)>.01) {
        s->pendulum_angle=clamp(s->pendulum_angle+horizontal*.006,-.95,.95);
        s->pendulum_manual=1; ++s->revision;
    }
}
void exhibits_update(ExhibitsState *s,double dt) {
    if (dt<=0) return;
    double old_open=s->sculpture_open,old_angle=s->pendulum_angle,old_gears=s->gear_phase,old_x=s->green_x;
    double target=s->held==TARGET_SCULPTURE ? 1 : 0;
    s->sculpture_open+=(target-s->sculpture_open)*(1-exp(-4*dt));
    if (fabs(target-s->sculpture_open)<.001) s->sculpture_open=target;
    if (s->held==TARGET_PENDULUM) {
        if (!s->pendulum_manual) s->pendulum_angle=clamp(s->pendulum_angle+.6*dt,-.95,.95);
        s->pendulum_velocity=0;
    } else if (fabs(s->pendulum_angle)>.0002 || fabs(s->pendulum_velocity)>.0002) {
        int steps=(int)ceil(dt/.008); double h=dt/steps;
        for (int i=0;i<steps;++i) {
            s->pendulum_velocity+=(-9.81/PENDULUM_LENGTH*sin(s->pendulum_angle)-.38*s->pendulum_velocity)*h;
            s->pendulum_angle+=s->pendulum_velocity*h;
        }
    } else { s->pendulum_angle=0; s->pendulum_velocity=0; }
    if (s->held>=TARGET_GEAR_0 && s->held<=TARGET_GEAR_2) {
        int index=s->held-TARGET_GEAR_0;
        double ratio=(index==1 ? -1 : 1)*16.0/gear_teeth(index);
        s->gear_phase=fmod(s->gear_phase+60*dt/ratio,5400);
    }
    if (s->held==TARGET_RGB_LAMP) {
        double range=3.8,phase=s->green_x-2.6;
        if (s->green_direction<0) phase=2*range-phase;
        phase=fmod(phase+1.2*dt,2*range);
        if (phase<range) { s->green_x=2.6+phase; s->green_direction=1; }
        else { s->green_x=6.4-(phase-range); s->green_direction=-1; }
    }
    if (old_open!=s->sculpture_open || old_angle!=s->pendulum_angle || old_gears!=s->gear_phase || old_x!=s->green_x) ++s->revision;
}
Vec3 sculpture_piece(const ExhibitsState *s,int index) {
    double gap=.43*(1+2*s->sculpture_open);
    return V(SCULPTURE_X+(index%3-1)*gap,SCULPTURE_Y+((index/3)%3-1)*gap+.95*s->sculpture_open,
             SCULPTURE_Z+(index/9-1)*gap);
}
Vec3 pendulum_bob(const ExhibitsState *s) {
    return V(PENDULUM_X+PENDULUM_LENGTH*sin(s->pendulum_angle),PENDULUM_Y-PENDULUM_LENGTH*cos(s->pendulum_angle),PENDULUM_Z);
}
Vec3 gear_center(int index) {
    static const Vec3 centers[]={{-5.3,3,GEAR_Z},{-3.74,3,GEAR_Z},{-2.596846467,3.66,GEAR_Z}};
    return centers[index];
}
double gear_radius(int index) { static const double r[]={.96,.60,.72}; return r[index]; }
int gear_teeth(int index) { static const int n[]={16,10,12}; return n[index]; }
double gear_angle(const ExhibitsState *s,int index) {
    static const double phase[]={0,18,15};
    return fmod((index==1 ? -1 : 1)*s->gear_phase*16/gear_teeth(index)+phase[index],360);
}
Vec3 rgb_button(int channel) { return V(3.5+channel,1.55,-10.7); }
Vec3 rgb_lamp(const ExhibitsState *s,int channel) { return V(channel==0 ? 2.8 : channel==1 ? s->green_x : 6.4,channel==1 ? RGB_GREEN_Y : 3.65,-12.5); }
int exhibits_collision(double x,double z) {
    if (fabs(x-SCULPTURE_X)<1.8 && fabs(z-SCULPTURE_Z)<1.8) return 1;
    if (fabs(x-PENDULUM_X)<3.05 && fabs(z-PENDULUM_Z)<.8) return 1;
    if (x>-6.8 && x<-.95 && z>-15.65 && z<-14.05) return 1;
    if (x>1.25 && x<7.65 && z<-10.15 && z>-18.1) return 1;
    return 0;
}

int exhibits_self_test(void) {
#define CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA obras: %s\n",msg); return EXIT_FAILURE; } } while (0)
    ExhibitsState s={0}; exhibits_reset(&s);
    Vec3 initial=sculpture_piece(&s,26);
    exhibits_press(&s,TARGET_SCULPTURE); exhibits_update(&s,2);
    Vec3 expanded=sculpture_piece(&s,26);
    CHECK(expanded.x>initial.x+.8 && expanded.y>initial.y+1 && expanded.z>initial.z+.8,"translacao em tres eixos");
    exhibits_release(&s); exhibits_update(&s,3);
    CHECK(s.sculpture_open==0,"escultura retorna ao soltar");
    exhibits_press(&s,TARGET_PENDULUM); exhibits_update(&s,1);
    CHECK(s.pendulum_angle>.5 && s.pendulum_velocity==0,"segurar puxa pendulo");
    Vec3 bob=pendulum_bob(&s);
    CHECK(fabs(hypot(bob.x-PENDULUM_X,bob.y-PENDULUM_Y)-PENDULUM_LENGTH)<1e-7,"comprimento fixo");
    exhibits_drag(&s,-150); double pulled=s.pendulum_angle;
    exhibits_release(&s); exhibits_update(&s,.6);
    CHECK(s.pendulum_angle!=pulled && fabs(s.pendulum_angle)<fabs(pulled),"pendulo oscila e amortece");
    for (int i=0;i<800;++i) exhibits_update(&s,.1);
    CHECK(s.pendulum_angle==0,"pendulo repousa apos amortecimento");
    exhibits_press(&s,TARGET_GEAR_0); exhibits_update(&s,.5);
    CHECK(fabs(gear_angle(&s,0)-30)<1e-7 && fabs(gear_angle(&s,1)+30)<1e-7 && fabs(gear_angle(&s,2)-55)<1e-7,"engrenagens com razoes e sentidos opostos");
    exhibits_release(&s); double phase=s.gear_phase; exhibits_update(&s,.5);
    CHECK(s.gear_phase==phase,"engrenagens param ao soltar");
    exhibits_press(&s,TARGET_RGB_R);
    CHECK(!s.rgb[0] && s.rgb[1] && s.rgb[2] && s.held==-1,"interruptores RGB independentes");
    exhibits_press(&s,TARGET_RGB_LAMP); exhibits_update(&s,1);
    CHECK(s.green_x>4.5 && s.green_x<=6.4,"translacao da fonte verde");
    exhibits_update(&s,100);
    CHECK(s.green_x>=2.6 && s.green_x<=6.4,"fonte permanece nos limites");
    CHECK(!exhibits_collision(0,5) && !exhibits_collision(0,-14) && !exhibits_collision(0,-4),"rota central livre");
    puts("OK: escultura, pendulo amortecido, engrenagens, controles RGB e circulacao.");
    return EXIT_SUCCESS;
#undef CHECK
}
