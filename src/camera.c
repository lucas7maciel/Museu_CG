/* Camera em primeira pessoa e movimento por mouse/teclado. */
#include "museum.h"
#include "building.h"
#include "camera.h"

Vec3 look_direction(void) {
    double a=museum.yaw*PI/180,b=museum.pitch*PI/180;
    return V(cos(a)*cos(b),sin(b),sin(a)*cos(b));
}

void move_player(double dt, int forward, int side, int running) {
    double a = museum.yaw * PI / 180;
    double dx = cos(a)*forward - sin(a)*side, dz = sin(a)*forward + cos(a)*side;
    double length = hypot(dx, dz), speed = running ? 5 : 3;
    if (length > 0) {
        /* Subpassos evitam saltar paredes/degraus em quadros demorados. */
        int steps = (int)ceil(speed*dt/.08);
        for (int i = 0; i < steps; ++i) {
            double sx = dx/length*speed*dt/steps, sz = dz/length*speed*dt/steps;
            if (walkable(museum.position.x+sx, museum.position.z, ground_height(museum.position.x,museum.position.z))) museum.position.x += sx;
            if (walkable(museum.position.x, museum.position.z+sz, ground_height(museum.position.x,museum.position.z))) museum.position.z += sz;
        }
    }
    museum.position.y = ground_height(museum.position.x,museum.position.z)+HEIGHT;
}

void keyboard_look(double dt,int horizontal,int vertical) {
    if (museum.exhibits.held==TARGET_PENDULUM) { exhibits_drag(&museum.exhibits,horizontal*dt*180); return; }
    /* Independente da captura do mouse; velocidade em graus por segundo. */
    if (!horizontal && !vertical) return;
    museum.yaw+=horizontal*90*dt;
    museum.pitch=clamp(museum.pitch+vertical*90*dt,-85,85);
    museum.mouse_valid=0;
}

void mouse_callback(GLFWwindow *win, double x, double y) {
    (void)win;
    if (museum.exhibits.held==TARGET_PENDULUM) {
        if (museum.mouse_valid) exhibits_drag(&museum.exhibits,x-museum.mouse_x);
        museum.mouse_x=x; museum.mouse_y=y; museum.mouse_valid=1; return;
    }
    if (museum.earth_held || museum.balance.held>=0 || museum.exhibits.held>=0) { museum.mouse_valid=0; return; }
    if (museum.mouse_valid) {
        museum.yaw += (x-museum.mouse_x)*.11;
        museum.pitch = clamp(museum.pitch-(y-museum.mouse_y)*.11,-85,85);
    }
    museum.mouse_x = x; museum.mouse_y = y; museum.mouse_valid = 1;
}

void camera(int w, int h) {
    double aspect = (double)w/h;
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(65,aspect,.08,160);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    double a=museum.yaw*PI/180,b=museum.pitch*PI/180;
    gluLookAt(museum.position.x,museum.position.y,museum.position.z,museum.position.x+cos(a)*cos(b),
              museum.position.y+sin(b),museum.position.z+sin(a)*cos(b),0,1,0);
}
