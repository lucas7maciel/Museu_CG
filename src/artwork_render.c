/* Geometria visual das seis exposicoes do museu. */
#include "museum.h"
#include "render_internal.h"

void build_earth_mesh(void) {
    enum { LAT=48,LON=96 };
    int lat=museum.unlit_mode ? 24 : LAT,lon=museum.unlit_mode ? 48 : LON;
    Vec3 colors[LAT+1][LON+1];
    for (int row=0;row<=lat;++row) for (int col=0;col<=lon;++col) {
        double latitude=-PI/2+PI*row/lat,longitude=-PI+2*PI*col/lon;
        Vec3 n=V(cos(latitude)*sin(longitude),sin(latitude),cos(latitude)*cos(longitude));
        Vec3 p=V(EARTH_X+EARTH_RADIUS*n.x,EARTH_Y+EARTH_RADIUS*n.y,EARTH_Z+EARTH_RADIUS*n.z);
        /* Pequena parcela de preenchimento permite ler os continentes no lado escuro. */
        colors[row][col]=render_color(p,n,V(1,1,1),.12);
    }
    if (!renderer.earth_scene_list) renderer.earth_scene_list=glGenLists(1);
    if (renderer.earth_scene_list) glNewList(renderer.earth_scene_list,GL_COMPILE);
    material(V(1,1,1),T_EARTH,16);
    for (int row=0;row<lat;++row) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int col=0;col<=lon;++col) for (int k=0;k<2;++k) {
            int r=row+k;
            double latitude=-PI/2+PI*r/lat,longitude=-PI+2*PI*col/lon;
            Vec3 n=V(cos(latitude)*sin(longitude),sin(latitude),cos(latitude)*cos(longitude));
            Vec3 color=colors[r][col]; glColor3d(color.x,color.y,color.z);
            glTexCoord2d((double)col/lon,1-(double)r/lat);
            glVertex3d(EARTH_X+EARTH_RADIUS*n.x,EARTH_Y+EARTH_RADIUS*n.y,EARTH_Z+EARTH_RADIUS*n.z);
        }
        glEnd();
    }
    if (renderer.earth_scene_list) glEndList();
}

void draw_earth(void) {
    /* Esfera simetrica: deslocar U equivale a gira-la em Y e preserva a luz no mundo. */
    glMatrixMode(GL_TEXTURE); glPushMatrix(); glTranslated(-museum.earth_angle/360,0,0);
    glMatrixMode(GL_MODELVIEW);
    if (renderer.earth_scene_list) glCallList(renderer.earth_scene_list);
    else build_earth_mesh();
    glMatrixMode(GL_TEXTURE); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
}

void draw_balance(void) {
    Vec3 metal=V(.36,.41,.45),beam=V(.7,.53,.26),pan=V(.57,.61,.64);
    renderer.object_fill=.065;
    box(V(BALANCE_X,FLOOR_Y+.12,BALANCE_Z),V(1.4,.24,1.2),metal,T_NONE,16);
    box(V(BALANCE_X,(FLOOR_Y+.24+BALANCE_PIVOT_Y)/2,BALANCE_Z),
        V(.2,BALANCE_PIVOT_Y-FLOOR_Y-.24,.2),metal,T_NONE,16);
    object_begin_roll(V(BALANCE_X,BALANCE_PIVOT_Y,BALANCE_Z),museum.balance.angle);
    box(V(0,0,0),V(2*BALANCE_ARM+.18,.14,.24),beam,T_NONE,32);
    object_end();
    box(V(BALANCE_X,BALANCE_PIVOT_Y,BALANCE_Z),V(.32,.32,.40),metal,T_NONE,32);
    for (int i=0;i<2;++i) {
        Vec3 end=balance_endpoint(&museum.balance,i);
        double plate_y=end.y-BALANCE_DROP;
        box(end,V(.16,.12,2.1),beam,T_NONE,32);
        for (int side=-1;side<=1;side+=2)
            box(V(end.x,(end.y+plate_y)/2,end.z+side*.98),V(.045,BALANCE_DROP,.045),pan,T_NONE,32);
        box(V(end.x,plate_y,end.z),V(1.9,.10,2.12),pan,T_NONE,32);
        draw_sphere(balance_ball_center(&museum.balance,i),museum.balance.radius[i],i==0 ? V(.95,.21,.12) : V(.12,.4,.98));
    }
    renderer.object_fill=0;
}

static Vec3 rgb_color(int channel) {
    return channel==0 ? V(1,.04,.025) : channel==1 ? V(.025,1,.05) : V(.035,.15,1);
}

static void draw_rgb_lamp(int channel) {
    Vec3 p=rgb_lamp(&museum.exhibits,channel);
    box(V(p.x,(FLOOR_Y+p.y-.2)/2,p.z),V(.065,p.y-.2-FLOOR_Y,.065),V(.2,.24,.28),T_NONE,16);
    if (renderer.collecting_shadows) return; /* A carcaca nao cobre a propria fonte pontual. */
    renderer.material_emission=.13;
    box(p,V(.42,.32,.42),channel==0 ? V(.45,.10,.08) : channel==1 ? V(.07,.45,.12) : V(.07,.15,.45),T_NONE,16);
    renderer.material_emission=museum.exhibits.rgb[channel] ? 1.25 : .06;
    box(V(p.x,p.y,p.z-.23),V(.32,.23,.035),rgb_color(channel),T_NONE,8);
    renderer.material_emission=0;
}

void draw_exhibit_static(void) {
    renderer.object_fill=.045;
    box(V(SCULPTURE_X,FLOOR_Y+.475,SCULPTURE_Z),V(1.6,.95,1.6),V(.22,.27,.30),T_NONE,16);
    for (int side=-1;side<=1;side+=2) {
        box(V(PENDULUM_X+side*2.6,(FLOOR_Y+PENDULUM_Y)/2,PENDULUM_Z),V(.13,PENDULUM_Y-FLOOR_Y,.18),V(.5,.38,.22),T_NONE,16);
        box(V(PENDULUM_X+side*2.6,FLOOR_Y+.06,PENDULUM_Z),V(.45,.12,1),V(.25,.29,.32),T_NONE,16);
    }
    box(V(PENDULUM_X,PENDULUM_Y,PENDULUM_Z),V(5.4,.16,.22),V(.6,.45,.23),T_NONE,16);
    box(V(-4.1,FLOOR_Y+.12,-15),V(5.25,.24,1.2),V(.25,.28,.32),T_NONE,16);
    box(V(-4.1,2.85,-15.16),V(5.25,3.7,.14),V(.09,.13,.17),T_NONE,16);
    for (int i=0;i<3;++i) {
        Vec3 c=gear_center(i);
        box(V(c.x,c.y,GEAR_Z-.20),V(.14,.14,.6),V(.45,.46,.48),T_NONE,16);
    }
    double old_step=renderer.geometry_step; renderer.geometry_step=.20;
    box(V(RGB_X,3.05,RGB_SCREEN_Z-.1),V(6.2,4.1,.12),V(.10,.13,.16),T_NONE,8);
    box(V(RGB_X,3.05,RGB_SCREEN_Z),V(5.9,3.8,.08),V(.94,.94,.94),T_NONE,8);
    box(V(RGB_X,FLOOR_Y+.18,-15.5),V(4.4,.36,1.8),V(.30,.33,.35),T_NONE,16);
    box(V(3.35,1.75,-15.5),V(.85,1.35,.8),V(.94,.94,.94),T_NONE,16);
    box(V(5.85,1.95,-15.5),V(.5,1.75,.5),V(.94,.94,.94),T_NONE,16);
    box(V(4.6,1.13,-15.4),V(.3,.15,.3),V(.85,.85,.85),T_NONE,16);
    draw_sphere(V(4.6,1.9,-15.4),.72,V(.95,.95,.95));
    renderer.geometry_step=old_step;
    box(V(RGB_X,FLOOR_Y+.38,-10.75),V(3.65,.76,.7),V(.17,.21,.24),T_NONE,16);
    for (int i=0;i<3;++i) {
        if (!renderer.collecting_shadows) renderer.material_emission=museum.exhibits.rgb[i] ? .7 : .04;
        box(rgb_button(i),V(.64,.20,.45),rgb_color(i),T_NONE,8);
        renderer.material_emission=0;
        if (i!=1) draw_rgb_lamp(i);
    }
    renderer.object_fill=0;
}

void draw_exhibit_dynamic(void) {
    renderer.object_fill=.16;
    for (int i=0;i<27;++i)
        box(sculpture_piece(&museum.exhibits,i),V(.40,.40,.40),V(.23+.25*(i%3),.30+.20*((i/3)%3),.26+.27*(i/9)),T_NONE,16);
    renderer.object_fill=.055;
    object_begin_roll(V(PENDULUM_X,PENDULUM_Y,PENDULUM_Z),museum.exhibits.pendulum_angle*180/PI);
    box(V(0,-PENDULUM_LENGTH/2,0),V(.055,PENDULUM_LENGTH,.055),V(.7,.7,.64),T_NONE,32);
    object_end();
    draw_sphere(pendulum_bob(&museum.exhibits),PENDULUM_RADIUS,V(.95,.59,.12));
    for (int g=0;g<3;++g) {
        double radius=gear_radius(g),angle=gear_angle(&museum.exhibits,g);
        Vec3 center=gear_center(g),base=g==0 ? V(.82,.52,.20) : g==1 ? V(.27,.65,.79) : V(.74,.32,.22);
        object_begin_roll(center,angle);
        ring_z(radius*.50,radius*.90,.22,base);
        ring_z(0,radius*.23,.28,V(.6,.65,.68));
        object_end();
        for (int tooth=0;tooth<gear_teeth(g);++tooth) {
            object_begin_roll(center,angle+tooth*360.0/gear_teeth(g));
            box(V(radius,0,0),V(.22,.18,.22),base,T_NONE,32);
            object_end();
        }
        for (int spoke=0;spoke<4;++spoke) {
            object_begin_roll(center,angle+spoke*90);
            box(V(radius*.38,0,0),V(radius*.50,.09,.18),base,T_NONE,32);
            object_end();
        }
    }
    draw_rgb_lamp(1);
    renderer.object_fill=0;
}
