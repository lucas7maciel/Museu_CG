/* Selecao pela mira, eventos e atualizacao das obras. */
#include "museum.h"
#include "building.h"
#include "camera.h"
#include "interaction.h"

static int sphere_hit(Vec3 origin,Vec3 direction,Vec3 center,double radius,double *distance) {
    Vec3 p=V(origin.x-center.x,origin.y-center.y,origin.z-center.z);
    double b=p.x*direction.x+p.y*direction.y+p.z*direction.z;
    double c=p.x*p.x+p.y*p.y+p.z*p.z-radius*radius,discriminant=b*b-c;
    if (discriminant<0) return 0;
    double t=-b-sqrt(discriminant);
    if (t<0) t=-b+sqrt(discriminant);
    if (t<0 || t>GATE_REACH) return 0;
    *distance=t;
    return 1;
}

static int earth_hit(Vec3 origin,Vec3 direction,double *distance) {
    return sphere_hit(origin,direction,V(EARTH_X,EARTH_Y,EARTH_Z),EARTH_RADIUS,distance);
}

void update_earth(double dt) {
    double distance=sqrt((museum.position.x-EARTH_X)*(museum.position.x-EARTH_X)+(museum.position.y-EARTH_Y)*(museum.position.y-EARTH_Y)+
                         (museum.position.z-EARTH_Z)*(museum.position.z-EARTH_Z))-EARTH_RADIUS;
    if (distance>GATE_REACH) museum.earth_held=0;
    if (museum.earth_held) museum.earth_angle=fmod(museum.earth_angle+45*dt,360);
}

void update_balance(double dt) {
    if (museum.balance.held>=0) {
        Vec3 ball=balance_ball_center(&museum.balance,museum.balance.held);
        double distance=sqrt((museum.position.x-ball.x)*(museum.position.x-ball.x)+(museum.position.y-ball.y)*(museum.position.y-ball.y)+
            (museum.position.z-ball.z)*(museum.position.z-ball.z))-museum.balance.radius[museum.balance.held];
        if (distance>GATE_REACH) museum.balance.held=-1;
    }
    balance_update(&museum.balance,dt);
}

static int ray_box(Vec3 origin, Vec3 direction, Vec3 lo, Vec3 hi, double *distance) {
    double t_enter = 0, t_exit = GATE_REACH;
    double o[] = {origin.x,origin.y,origin.z}, d[] = {direction.x,direction.y,direction.z};
    double mn[] = {lo.x,lo.y,lo.z}, mx[] = {hi.x,hi.y,hi.z};
    for (int axis=0;axis<3;++axis) {
        if (fabs(d[axis])<1e-8) {
            if (o[axis]<mn[axis] || o[axis]>mx[axis]) return 0;
        } else {
            double a=(mn[axis]-o[axis])/d[axis], b=(mx[axis]-o[axis])/d[axis];
            t_enter=fmax(t_enter,fmin(a,b)); t_exit=fmin(t_exit,fmax(a,b));
            if (t_enter>t_exit) return 0;
        }
    }
    *distance=t_enter;
    return 1;
}

static int exhibit_hit(Vec3 origin,Vec3 direction,double *distance) {
    double nearest=GATE_REACH+1,hit; int target=-1;
    for (int i=0;i<27;++i) {
        Vec3 p=sculpture_piece(&museum.exhibits,i);
        if (ray_box(origin,direction,V(p.x-.2,p.y-.2,p.z-.2),V(p.x+.2,p.y+.2,p.z+.2),&hit) && hit<nearest) {
            nearest=hit; target=TARGET_SCULPTURE;
        }
    }
    if (sphere_hit(origin,direction,pendulum_bob(&museum.exhibits),PENDULUM_RADIUS,&hit) && hit<nearest) {
        nearest=hit; target=TARGET_PENDULUM;
    }
    for (int i=0;i<3;++i) {
        Vec3 p=gear_center(i); double radius=gear_radius(i)+.13;
        if (ray_box(origin,direction,V(p.x-radius,p.y-radius,p.z-.15),V(p.x+radius,p.y+radius,p.z+.15),&hit) && hit<nearest) {
            double x=origin.x+direction.x*hit-p.x,y=origin.y+direction.y*hit-p.y;
            if (x*x+y*y<=radius*radius) { nearest=hit; target=TARGET_GEAR_0+i; }
        }
        p=rgb_button(i);
        if (ray_box(origin,direction,V(p.x-.32,p.y-.10,p.z-.225),V(p.x+.32,p.y+.10,p.z+.225),&hit) && hit<nearest) {
            nearest=hit; target=TARGET_RGB_R+i;
        }
        p=rgb_lamp(&museum.exhibits,i);
        if (ray_box(origin,direction,V(p.x-.24,p.y-.20,p.z-.27),V(p.x+.24,p.y+.20,p.z+.27),&hit) && hit<nearest) {
            nearest=hit; target=i==1 ? TARGET_RGB_LAMP : TARGET_RGB_R+i;
        }
    }
    *distance=nearest;
    return target;
}

int interaction_target(Vec3 origin, Vec3 direction) {
    double nearest=GATE_REACH+1, hit;
    int target=-1;
    /* O vao continua clicavel com as folhas abertas. */
    if (ray_box(origin,direction,V(-2.35,FLOOR_Y,GATE_Z-.14),V(2.35,FLOOR_Y+3.5,GATE_Z+.14),&hit)) nearest=hit;
    for (int side=0;side<2;++side) {
        Vec3 o=gate_local(origin,side,museum.gate_angle,0), d=gate_local(direction,side,museum.gate_angle,1);
        double sign=side == 0 ? 1 : -1;
        o.x*=sign; d.x*=sign;
        if (ray_box(o,d,V(0,0,-GATE_T/2),V(GATE_WIDTH,GATE_HEIGHT,GATE_T/2),&hit)) nearest=fmin(nearest,hit);
    }
    if (nearest<=GATE_REACH) target=0;
    for (int i=0;i<WINDOW_COUNT;++i) {
        Opening o=window_opening(i);
        double x=(i<3 ? -1 : 1)*8.2;
        double distance=GATE_REACH+1;
        if (ray_box(origin,direction,V(x-.14,FLOOR_Y+o.sill,o.start),V(x+.14,FLOOR_Y+o.head,o.end),&hit)) distance=hit;
        for (int leaf=0;leaf<2;++leaf) {
            Vec3 p=window_local(origin,i,leaf,museum.windows[i].angle,0);
            Vec3 d=window_local(direction,i,leaf,museum.windows[i].angle,1);
            double sign=leaf ? -1 : 1;
            p.z*=sign; d.z*=sign;
            if (ray_box(p,d,V(-WINDOW_T/2,0,0),V(WINDOW_T/2,o.head-o.sill-.08,window_width(i)),&hit)) distance=fmin(distance,hit);
        }
        if (distance<nearest) { nearest=distance; target=i+1; }
    }
    if (earth_hit(origin,direction,&hit) && hit<nearest) { nearest=hit; target=TARGET_EARTH; }
    for (int i=0;i<2;++i)
        if (sphere_hit(origin,direction,balance_ball_center(&museum.balance,i),museum.balance.radius[i],&hit) && hit<nearest) {
            nearest=hit; target=TARGET_BALANCE_LEFT+i;
        }
    int extra=exhibit_hit(origin,direction,&hit);
    if (extra>=0 && hit<nearest) { nearest=hit; target=extra; }
    if (target<0) return -1;
    size_t wall_count;
    const Solid *walls=building_walls(&wall_count);
    for (size_t i=0;i<wall_count;++i) {
        Solid s=walls[i];
        if (ray_box(origin,direction,V(s.center.x-s.size.x/2,s.center.y-s.size.y/2,s.center.z-s.size.z/2),
                    V(s.center.x+s.size.x/2,s.center.y+s.size.y/2,s.center.z+s.size.z/2),&hit) && hit<nearest-.001) return -1;
    }
    return target;
}

int aimed_target(void) {
    return interaction_target(museum.position,look_direction());
}

void mouse_button_callback(GLFWwindow *win, int button, int action, int mods) {
    (void)win; (void)mods;
    if (button!=GLFW_MOUSE_BUTTON_LEFT) return;
    if (action==GLFW_RELEASE) { museum.earth_held=0; museum.balance.held=-1; exhibits_release(&museum.exhibits); return; }
    if (action!=GLFW_PRESS) return;
    Vec3 origin=museum.position, direction=look_direction();
    int target=interaction_target(origin,direction);
    if (target!=TARGET_EARTH) museum.earth_held=0;
    if (target!=TARGET_BALANCE_LEFT && target!=TARGET_BALANCE_RIGHT) museum.balance.held=-1;
    if (target<TARGET_SCULPTURE || target>TARGET_RGB_LAMP) exhibits_release(&museum.exhibits);
    if (target==0) museum.gate_open=!museum.gate_open;
    else if (target==TARGET_EARTH) { museum.earth_held=1; museum.mouse_valid=0; }
    else if (target==TARGET_BALANCE_LEFT || target==TARGET_BALANCE_RIGHT) { museum.balance.held=target-TARGET_BALANCE_LEFT; museum.mouse_valid=0; }
    else if (target>=TARGET_SCULPTURE && target<=TARGET_RGB_LAMP) { exhibits_press(&museum.exhibits,target); museum.mouse_valid=0; }
    else if (target>0 && target<=WINDOW_COUNT) museum.windows[target-1].open=!museum.windows[target-1].open;
}

void focus_callback(GLFWwindow *win,int focused) {
    (void)win;
    if (!focused) { museum.earth_held=0; museum.balance.held=-1; exhibits_release(&museum.exhibits); }
    museum.mouse_valid=0;
}

void update_exhibits(double dt) {
    int held=museum.exhibits.held;
    if (held>=0) {
        Vec3 center=V(SCULPTURE_X,SCULPTURE_Y,SCULPTURE_Z); double radius=1.8;
        if (held==TARGET_PENDULUM) { center=pendulum_bob(&museum.exhibits); radius=PENDULUM_RADIUS; }
        else if (held>=TARGET_GEAR_0 && held<=TARGET_GEAR_2) { center=gear_center(held-TARGET_GEAR_0); radius=gear_radius(held-TARGET_GEAR_0); }
        else if (held==TARGET_RGB_LAMP) { center=rgb_lamp(&museum.exhibits,1); radius=.3; }
        double distance=sqrt((museum.position.x-center.x)*(museum.position.x-center.x)+(museum.position.y-center.y)*(museum.position.y-center.y)+
            (museum.position.z-center.z)*(museum.position.z-center.z))-radius;
        if (distance>GATE_REACH) exhibits_release(&museum.exhibits);
    }
    exhibits_update(&museum.exhibits,dt);
}

void key_callback(GLFWwindow *win, int key, int scancode, int action, int mods) {
    (void)win; (void)scancode; (void)mods;
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_R) { balance_reset(&museum.balance); exhibits_reset(&museum.exhibits); }
    if (key == GLFW_KEY_F2) museum.unlit_mode=!museum.unlit_mode;
    if (key == GLFW_KEY_L) museum.sunlight_on = !museum.sunlight_on;
    if (key == GLFW_KEY_1) ceiling_lights_on = !ceiling_lights_on;
    if (key == GLFW_KEY_2) corridor_lights_on = !corridor_lights_on;
    if (key == GLFW_KEY_3) earth_light_on = !earth_light_on;
    if (key == GLFW_KEY_7) exhibit_lights_on = !exhibit_lights_on;
    if (key>=GLFW_KEY_4 && key<=GLFW_KEY_6) { int channel=key-GLFW_KEY_4; museum.exhibits.rgb[channel]=!museum.exhibits.rgb[channel]; ++museum.exhibits.revision; }
    museum.mouse_valid = 0;
}
