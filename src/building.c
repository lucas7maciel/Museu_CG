/* Planta, paredes, escadas, cobertura, folhas e colisoes. */
#include "museum.h"
#include "building.h"
#include "render_internal.h"
#include <stdio.h>
#include <stdlib.h>

/* Planta: entrada em +Z, salao principal, corredor e salao posterior em -Z. */
static const Rect rooms[] = {
    {-8.2, 8.2, 0, 20}, {-2.1, 2.1, -7.6, 0}, {-8.2, 8.8, -20, -7.6}
};
static const Opening left_windows[] = {
    {2, 6.1, .95, 3.65}, {7.1, 12, .95, 3.65}, {14.6, 18.7, .95, 3.65}
};
static const Opening right_windows[] = {
    {2.4, 5.8, .95, 3.65}, {8.1, 12, .95, 3.65}, {15.6, 18.2, .95, 3.65}
};
static const Opening entrance[] = {{-2.35, 2.35, 0, 3.5}};
static Solid walls[96];
static size_t wall_count;

static int earth_collision(double x,double z,double feet) {
    double y=clamp(EARTH_Y,feet,feet+HEIGHT+.12);
    return (x-EARTH_X)*(x-EARTH_X)+(z-EARTH_Z)*(z-EARTH_Z)+(y-EARTH_Y)*(y-EARTH_Y)<
        (EARTH_RADIUS+RADIUS)*(EARTH_RADIUS+RADIUS);
}

static int balance_collision(double x,double z) {
    if (fabs(x-BALANCE_X)<.7+RADIUS && fabs(z-BALANCE_Z)<.6+RADIUS) return 1;
    for (int i=0;i<2;++i) {
        Vec3 end=balance_endpoint(&museum.balance,i);
        if (fabs(x-end.x)<.95+RADIUS && fabs(z-end.z)<1.06+RADIUS) return 1;
    }
    return 0;
}

Vec3 gate_local(Vec3 p, int side, double angle, int direction) {
    double sign = side == 0 ? 1 : -1, a = sign*angle*PI/180;
    double c = cos(a), s = sin(a);
    if (!direction) { p.x += sign*GATE_HALF; p.y -= FLOOR_Y+.04; p.z -= GATE_Z; }
    return V(c*p.x-s*p.z, p.y, s*p.x+c*p.z);
}

int gate_collision(double x, double z, double feet, double height, double angle) {
    if (feet+height <= FLOOR_Y+.04 || feet >= FLOOR_Y+.04+GATE_HEIGHT) return 0;
    for (int side=0; side<2; ++side) {
        Vec3 p = gate_local(V(x,feet,z),side,angle,0);
        double u = (side == 0 ? 1 : -1)*p.x;
        if (hypot(u-clamp(u,0,GATE_WIDTH),p.z-clamp(p.z,-GATE_T/2,GATE_T/2)) < RADIUS) return 1;
    }
    return 0;
}

void update_gate(double dt) {
    double target = museum.gate_open ? 90 : 0;
    double remaining = clamp(target-museum.gate_angle,-90*dt,90*dt);
    museum.gate_blocked = 0;
    /* Verifica o arco em incrementos pequenos para nao atravessar o jogador. */
    while (fabs(remaining) > 1e-8) {
        double step = clamp(remaining,-1,1), next = museum.gate_angle+step;
        if (gate_collision(museum.position.x,museum.position.z,ground_height(museum.position.x,museum.position.z),HEIGHT+.12,next)) {
            museum.gate_blocked = 1;
            break;
        }
        museum.gate_angle = next;
        remaining -= step;
    }
}

void draw_gate(void) {
    Vec3 metal = V(.16,.21,.23), brass = V(.73,.54,.24);
    for (int side=0; side<2; ++side) {
        double sign = side == 0 ? 1 : -1;
        object_begin(V(-sign*GATE_HALF,FLOOR_Y+.04,GATE_Z),sign*museum.gate_angle);
        box(V(sign*GATE_WIDTH/2,.07,0),V(GATE_WIDTH,.14,GATE_T),metal,T_NONE,64);
        box(V(sign*GATE_WIDTH/2,GATE_HEIGHT-.07,0),V(GATE_WIDTH,.14,GATE_T),metal,T_NONE,64);
        box(V(sign*.05,GATE_HEIGHT/2,0),V(.1,GATE_HEIGHT,GATE_T),metal,T_NONE,64);
        box(V(sign*(GATE_WIDTH-.05),GATE_HEIGHT/2,0),V(.1,GATE_HEIGHT,GATE_T),metal,T_NONE,64);
        box(V(sign*GATE_WIDTH/2,.65,0),V(GATE_WIDTH-.16,1.04,.08),metal,T_NONE,48);
        box(V(sign*GATE_WIDTH/2,1.22,0),V(GATE_WIDTH,.1,GATE_T),metal,T_NONE,64);
        for (int bar=1;bar<=7;++bar)
            box(V(sign*GATE_WIDTH*bar/8,2.22,0),V(.055,1.94,.07),metal,T_NONE,64);
        for (int face=-1;face<=1;face+=2)
            box(V(sign*(GATE_WIDTH-.2),1.55,face*.11),V(.065,.38,.075),brass,T_NONE,80);
        for (int hinge=0;hinge<2;++hinge)
            box(V(0,.45+hinge*2.4,0),V(.17,.22,.2),brass,T_NONE,80);
        object_end();
    }
}

Opening window_opening(int index) {
    return index<3 ? left_windows[index] : right_windows[index-3];
}

double window_width(int index) {
    Opening o=window_opening(index);
    return (o.end-o.start-.08)/2-.015;
}

Vec3 window_hinge(int index,int leaf) {
    Opening o=window_opening(index);
    double exterior=index<3 ? -1 : 1;
    return V(exterior*(8.2+.09),FLOOR_Y+o.sill+.04,leaf ? o.end-.04 : o.start+.04);
}

static double window_rotation(int index,int leaf,double angle) {
    return (index<3 ? -1 : 1)*(leaf ? -1 : 1)*angle;
}

Vec3 window_local(Vec3 p,int index,int leaf,double angle,int direction) {
    Vec3 h=window_hinge(index,leaf);
    double a=window_rotation(index,leaf,angle)*PI/180,c=cos(a),s=sin(a);
    if (!direction) { p.x-=h.x; p.y-=h.y; p.z-=h.z; }
    return V(c*p.x-s*p.z,p.y,s*p.x+c*p.z);
}

Vec3 window_world(Vec3 p,int index,int leaf) {
    Vec3 h=window_hinge(index,leaf);
    double a=window_rotation(index,leaf,museum.windows[index].angle)*PI/180,c=cos(a),s=sin(a);
    return V(h.x+c*p.x+s*p.z,h.y+p.y,h.z-s*p.x+c*p.z);
}

int window_collision(int index,double x,double z,double feet,double height,double angle) {
    Opening o=window_opening(index);
    if (feet+height<=FLOOR_Y+o.sill+.04 || feet>=FLOOR_Y+o.head-.04) return 0;
    for (int leaf=0;leaf<2;++leaf) {
        Vec3 p=window_local(V(x,feet,z),index,leaf,angle,0);
        double u=(leaf ? -1 : 1)*p.z;
        if (hypot(p.x-clamp(p.x,-WINDOW_T/2,WINDOW_T/2),u-clamp(u,0,window_width(index)))<RADIUS) return 1;
    }
    return 0;
}

void update_windows(double dt) {
    for (int i=0;i<WINDOW_COUNT;++i) {
        WindowState *w=&museum.windows[i];
        double remaining=clamp((w->open ? 90 : 0)-w->angle,-90*dt,90*dt);
        w->blocked=0;
        while (fabs(remaining)>1e-8) {
            double step=clamp(remaining,-1,1),next=w->angle+step;
            if (window_collision(i,museum.position.x,museum.position.z,ground_height(museum.position.x,museum.position.z),HEIGHT+.12,next)) {
                w->blocked=1;
                break;
            }
            w->angle=next; remaining-=step;
        }
    }
}

void draw_windows(void) {
    Vec3 frame=V(.22,.29,.31),handle=V(.73,.54,.24);
    for (int i=0;i<WINDOW_COUNT;++i) {
        Opening o=window_opening(i);
        double x=(i<3 ? -1 : 1)*8.2,cy=FLOOR_Y+(o.sill+o.head)/2;
        double cz=(o.start+o.end)/2,height=o.head-o.sill-.08,width=window_width(i);
        /* Batente fixo encaixado no vao, sem alterar a planta. */
        for (int edge=0;edge<2;++edge) {
            box(V(x,FLOOR_Y+(edge ? o.head-.04 : o.sill+.04),cz),V(.22,.08,o.end-o.start),frame,T_NONE,48);
            box(V(x,cy,edge ? o.end-.04 : o.start+.04),V(.22,o.head-o.sill,.08),frame,T_NONE,48);
        }
        for (int leaf=0;leaf<2;++leaf) {
            double sign=leaf ? -1 : 1;
            Vec3 h=window_hinge(i,leaf);
            object_begin(h,window_rotation(i,leaf,museum.windows[i].angle));
            /* Painel opaco com espessura, solidario a folha animada. */
            box(V(0,height/2,sign*width/2),V(.06,height-.16,width-.16),V(.46,.55,.57),T_NONE,16);
            for (int edge=0;edge<2;++edge) {
                box(V(0,edge ? height-.045 : .045,sign*width/2),V(WINDOW_T,.09,width),frame,T_NONE,64);
                box(V(0,height/2,sign*(edge ? width-.045 : .045)),V(WINDOW_T,height,.09),frame,T_NONE,64);
            }
            box(V(0,height/2,sign*width/2),V(.07,.055,width-.16),frame,T_NONE,64);
            for (int face=-1;face<=1;face+=2)
                box(V(face*.09,height*.45,sign*(width-.18)),V(.07,.28,.055),handle,T_NONE,80);
            object_end();
        }
    }
}

static void wall_piece(int along_x, double fixed, double start, double end, double bottom, double top) {
    if (end <= start || top <= bottom) return;
    if (wall_count >= COUNT(walls)) { fputs("Limite de paredes excedido.\n", stderr); exit(EXIT_FAILURE); }
    Solid *s = &walls[wall_count++];
    s->center = V(along_x ? (start+end)/2 : fixed, FLOOR_Y+(bottom+top)/2,
                  along_x ? fixed : (start+end)/2);
    s->size = V(along_x ? end-start : WALL_T, top-bottom, along_x ? WALL_T : end-start);
}

static void wall(int along_x, double fixed, double start, double end, const Opening *holes, size_t count) {
    double cursor = start;
    for (size_t i = 0; i < count; ++i) {
        const Opening *o = &holes[i];
        wall_piece(along_x, fixed, cursor, o->start, 0, WALL_HEIGHT);
        wall_piece(along_x, fixed, o->start, o->end, 0, o->sill);
        wall_piece(along_x, fixed, o->start, o->end, o->head, WALL_HEIGHT);
        cursor = o->end;
    }
    wall_piece(along_x, fixed, cursor, end, 0, WALL_HEIGHT);
}

void build_museum(void) {
    wall_count = 0;
    wall(0, -8.2, 0, 20, left_windows, COUNT(left_windows));
    wall(0, 8.2, 0, 20, right_windows, COUNT(right_windows));
    wall(1, 20, -8.2, 8.2, entrance, COUNT(entrance));
    wall(1, 0, -8.2, -2.1, NULL, 0);
    wall(1, 0, 2.1, 8.2, NULL, 0);
    wall(0, -2.1, -7.6, 0, NULL, 0);
    wall(0, 2.1, -7.6, 0, NULL, 0);
    wall(1, -7.6, -8.2, -2.1, NULL, 0);
    wall(1, -7.6, 2.1, 8.8, NULL, 0);
    wall(0, -8.2, -20, -7.6, NULL, 0);
    wall(0, 8.8, -20, -7.6, NULL, 0);
    wall(1, -20, -8.2, 8.8, NULL, 0);
}

static Rect stair_rect(int i) {
    return (Rect){-5.75, 5.75, 20+WALL_T/2+i*.95, 20+WALL_T/2+(i+1)*.95};
}

static int contains(Rect r, double x, double z) {
    return x >= r.x0 && x <= r.x1 && z >= r.z0 && z <= r.z1;
}

static Rect floor_rect(size_t i) {
    Rect r = rooms[i];
    r.x0 -= WALL_T/2; r.x1 += WALL_T/2;
    r.z0 -= WALL_T/2; r.z1 += WALL_T/2;
    /* Encontra os pisos dos saloes sem sobrepor faces coplanares. */
    if (i == 1) { r.z0 = rooms[2].z1+WALL_T/2; r.z1 = rooms[0].z0-WALL_T/2; }
    return r;
}

double ground_height(double x, double z) {
    for (size_t i = 0; i < COUNT(rooms); ++i) {
        Rect r = floor_rect(i);
        if (contains(r, x, z)) return FLOOR_Y;
    }
    for (int i = 0; i < 4; ++i)
        if (contains(stair_rect(i), x, z)) return FLOOR_Y - i*.18;
    return 0;
}

int wall_collision(double x, double z, double feet, double height) {
    for (size_t i = 0; i < wall_count; ++i) {
        Solid s = walls[i];
        if (s.center.y+s.size.y/2 <= feet+.02 || s.center.y-s.size.y/2 >= feet+height) continue;
        double nx = clamp(x, s.center.x-s.size.x/2, s.center.x+s.size.x/2);
        double nz = clamp(z, s.center.z-s.size.z/2, s.center.z+s.size.z/2);
        if (hypot(x-nx, z-nz) < RADIUS) return 1;
    }
    return 0;
}

int walkable(double x, double z, double from_height) {
    if (fabs(x) >= 37 || z <= -35 || z >= 38) return 0;
    double floor = ground_height(x, z);
    /* A lateral da plataforma nao pode ser escalada; acesso pelos degraus. */
    if (fabs(floor-from_height) > .181) return 0;
    if (earth_collision(x,z,floor)) return 0;
    if (balance_collision(x,z)) return 0;
    if (exhibits_collision(x,z)) return 0;
    if (wall_collision(x,z,floor,HEIGHT+.12) || gate_collision(x,z,floor,HEIGHT+.12,museum.gate_angle)) return 0;
    for (int i=0;i<WINDOW_COUNT;++i)
        if (window_collision(i,x,z,floor,HEIGHT+.12,museum.windows[i].angle)) return 0;
    return 1;
}

static void draw_roof(void) {
    /* Laje plana de 0,32: a face inferior forma o teto interno em ROOF_Y. */
    for (size_t i=0;i<COUNT(rooms);++i) {
        Rect r=floor_rect(i);
        box(V((r.x0+r.x1)/2,ROOF_Y+.16,(r.z0+r.z1)/2),
            V(r.x1-r.x0,.32,r.z1-r.z0),V(.78,.78,.73),T_NONE,8);
    }
}

void static_geometry(void) {
    if (!renderer.collecting_shadows) {
        material(V(.8,.85,.77),T_GRASS,4);
        glBegin(GL_QUADS);
        int rows=museum.unlit_mode ? 1 : 39,cols=museum.unlit_mode ? 1 : 40;
        for (int row=0;row<rows;++row) for (int col=0;col<cols;++col) for (int k=0;k<4;++k) {
            double x=-40+80.0*(col+(k==1 || k==2))/cols,z=-38+78.0*(row+(k>=2))/rows;
            Vec3 color=render_color(V(x,-.015,z),V(0,1,0),V(.8,.85,.77),0);
            glColor3d(color.x,color.y,color.z); glTexCoord2d((x+40)/2,(z+38)/2); glVertex3d(x,-.015,z);
        }
        glEnd();
    }
    box(V(0,-.005,30.5),V(11.5,.02,13),V(.81,.8,.75),T_STONE,12);
    for (size_t i = 0; i < COUNT(rooms); ++i) {
        Rect r = floor_rect(i);
        box(V((r.x0+r.x1)/2,FLOOR_Y/2,(r.z0+r.z1)/2),
            V(r.x1-r.x0,FLOOR_Y,r.z1-r.z0),V(.73,.74,.71),T_STONE,30);
    }
    for (int i = 0; i < 4; ++i) {
        Rect r = stair_rect(i);
        double h = FLOOR_Y-i*.18;
        box(V(0,h/2,(r.z0+r.z1)/2),V(r.x1-r.x0,h,r.z1-r.z0),V(.79,.76,.66),T_STONE,16);
    }
    for (size_t i = 0; i < wall_count; ++i)
        box(walls[i].center,walls[i].size,V(.9,.88,.81),T_NONE,12);
    draw_roof();
}

const Solid *building_walls(size_t *count) {
    *count=wall_count;
    return walls;
}
