/* Primitivas de desenho, transformacoes, materiais e texturas. */
#include "museum.h"
#include "render_internal.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

RenderState renderer={.geometry_step=.60};
static GLuint textures[T_COUNT];
static Vec3 object_origin;
static double object_angle,object_cos=1,object_sin;
static double object_roll,object_roll_cos=1,object_roll_sin;

Vec3 render_color(Vec3 point,Vec3 normal,Vec3 color,double emission) {
    if (museum.unlit_mode) return color;
    return lighting_sample(point,normal,color,emission,museum.sunlight_on);
}

Vec3 object_world(Vec3 p,int direction) {
    Vec3 result=V(object_cos*p.x+object_sin*p.z,p.y,-object_sin*p.x+object_cos*p.z);
    if (object_roll!=0) result=V(object_roll_cos*p.x-object_roll_sin*p.y,object_roll_sin*p.x+object_roll_cos*p.y,p.z);
    if (!direction) { result.x+=object_origin.x; result.y+=object_origin.y; result.z+=object_origin.z; }
    return result;
}

void object_begin(Vec3 origin,double angle) {
    object_origin=origin; object_angle=angle;
    object_cos=cos(angle*PI/180); object_sin=sin(angle*PI/180);
    if (!renderer.collecting_shadows) { glPushMatrix(); glTranslated(origin.x,origin.y,origin.z); glRotated(angle,0,1,0); }
}

void object_end(void) {
    if (!renderer.collecting_shadows) glPopMatrix();
    object_origin=V(0,0,0); object_angle=0;
    object_cos=1; object_sin=0;
    object_roll=0; object_roll_cos=1; object_roll_sin=0;
}

void object_begin_roll(Vec3 origin,double angle) {
    object_begin(origin,0);
    object_roll=angle; object_roll_cos=cos(angle*PI/180); object_roll_sin=sin(angle*PI/180);
    if (!renderer.collecting_shadows) glRotated(angle,0,0,1);
}

static void make_texture(int id, int red, int green, int blue) {
    unsigned char pixels[64 * 64 * 3];
    uint32_t seed = 42;
    int base[3] = {red, green, blue};
    for (int y = 0; y < 64; ++y) for (int x = 0; x < 64; ++x) {
        seed = seed * 1664525u + 1013904223u;
        int noise = (int)((seed >> 16) % 15) - 7;
        if (id == T_STONE && (x == 0 || y == 0)) noise = -26;
        for (int c = 0; c < 3; ++c)
            pixels[(y * 64 + x) * 3 + c] = (unsigned char)clamp(base[c] + noise, 0, 255);
    }
    glGenTextures(1, &textures[id]);
    glBindTexture(GL_TEXTURE_2D, textures[id]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 64, 64, 0, GL_RGB, GL_UNSIGNED_BYTE, pixels);
}

static int load_earth_texture(const char *executable) {
    char path[2048];
    FILE *file=fopen("assets/earth.ppm","rb");
    if (!file) {
        const char *slash=strrchr(executable,'/'),*backslash=strrchr(executable,'\\');
        if (backslash && (!slash || backslash>slash)) slash=backslash;
        if (slash && (size_t)(slash-executable)+18<sizeof(path)) {
            snprintf(path,sizeof(path),"%.*s/assets/earth.ppm",(int)(slash-executable),executable);
            file=fopen(path,"rb");
        }
    }
    if (!file) { fputs("Textura ausente: mantenha assets/earth.ppm junto do executavel ou na pasta do projeto.\n",stderr); return 0; }
    char magic[3]; int width,height,max_value;
    if (fscanf(file,"%2s %d %d %d",magic,&width,&height,&max_value)!=4 || strcmp(magic,"P6") ||
        width<1 || width>8192 || height<1 || height>4096 || max_value!=255) { fclose(file); return 0; }
    int separator=fgetc(file);
    if (separator=='\r') { int next=fgetc(file); if (next!='\n') ungetc(next,file); }
    else if (separator!='\n' && separator!=' ' && separator!='\t') { fclose(file); return 0; }
    size_t length=(size_t)width*(size_t)height*3;
    unsigned char *pixels=malloc(length);
    if (!pixels) { fclose(file); return 0; }
    int ok=fread(pixels,1,length,file)==length;
    fclose(file);
    if (ok) {
        glGenTextures(1,&textures[T_EARTH]); glBindTexture(GL_TEXTURE_2D,textures[T_EARTH]);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);
        ok=gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels)==0;
    }
    free(pixels);
    return ok;
}

void material(Vec3 color, int tex, double shiny) {
    (void)shiny;
    if (renderer.collecting_shadows) return;
    glColor4d(color.x, color.y, color.z, 1);
    if (tex != T_NONE) {
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, textures[tex]);
    } else glDisable(GL_TEXTURE_2D);
}

void box(Vec3 pos, Vec3 size, Vec3 color, int tex, double shiny) {
    static const GLfloat normals[6][3] = {{0,0,1},{0,0,-1},{1,0,0},{-1,0,0},{0,1,0},{0,-1,0}};
    static const GLfloat vertices[6][4][3] = {
        {{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}},
        {{1,-1,-1},{-1,-1,-1},{-1,1,-1},{1,1,-1}},
        {{1,-1,1},{1,-1,-1},{1,1,-1},{1,1,1}},
        {{-1,-1,-1},{-1,-1,1},{-1,1,1},{-1,1,-1}},
        {{-1,1,1},{1,1,1},{1,1,-1},{-1,1,-1}},
        {{-1,-1,-1},{1,-1,-1},{1,-1,1},{-1,-1,1}}
    };
    if (renderer.collecting_shadows) {
        if (object_roll!=0) lighting_add_rolled_occluder(object_world(pos,0),size,object_roll);
        else lighting_add_occluder(object_world(pos,0),size,object_angle);
        return;
    }
    material(color, tex, shiny);
    glPushMatrix();
    glTranslated(pos.x, pos.y, pos.z);
    glScaled(size.x, size.y, size.z);
    glBegin(GL_QUADS);
    if (museum.unlit_mode) {
        /* Sem subdivisao, alocacao ou calculo de luz: seis faces por caixa. */
        for (int f=0;f<6;++f) {
            double u=(f<2 ? size.x : f<4 ? size.z : size.x)/2;
            double v=(f<4 ? size.y : size.z)/2;
            for (int k=0;k<4;++k) {
                glTexCoord2d((k==1 || k==2) ? u : 0,k>=2 ? v : 0);
                glVertex3f(vertices[f][k][0]*.5f,vertices[f][k][1]*.5f,vertices[f][k][2]*.5f);
            }
        }
        glEnd(); glPopMatrix(); return;
    }
    for (int f = 0; f < 6; ++f) {
        double u = (f < 2 ? size.x : f < 4 ? size.z : size.x) / 2;
        double v = (f < 4 ? size.y : size.z) / 2;
        Vec3 normal=object_world(V(normals[f][0],normals[f][1],normals[f][2]),1);
        double step=renderer.material_emission>0 ? 4 : renderer.geometry_step;
        int nu=(int)ceil(u*2/step),nv=(int)ceil(v*2/step);
        if (nu<1) nu=1;
        if (nv<1) nv=1;
        Vec3 *colors=malloc((size_t)(nu+1)*(size_t)(nv+1)*sizeof(*colors));
        if (!colors) { fputs("Memoria insuficiente para iluminacao.\n",stderr); exit(EXIT_FAILURE); }
        for (int row=0;row<=nv;++row) for (int col=0;col<=nu;++col) {
            double s=col/(double)nu,t=row/(double)nv;
            Vec3 p=V((vertices[f][0][0]+s*(vertices[f][1][0]-vertices[f][0][0])+t*(vertices[f][3][0]-vertices[f][0][0]))*.5,
                     (vertices[f][0][1]+s*(vertices[f][1][1]-vertices[f][0][1])+t*(vertices[f][3][1]-vertices[f][0][1]))*.5,
                     (vertices[f][0][2]+s*(vertices[f][1][2]-vertices[f][0][2])+t*(vertices[f][3][2]-vertices[f][0][2]))*.5);
            Vec3 world=object_world(V(pos.x+p.x*size.x,pos.y+p.y*size.y,pos.z+p.z*size.z),0);
            if (renderer.material_emission>=.8) {
                /* Difusores preservam a cor da fonte sem saturar pelo proprio foco. */
                colors[row*(nu+1)+col]=V(clamp(color.x*renderer.material_emission,0,1),clamp(color.y*renderer.material_emission,0,1),clamp(color.z*renderer.material_emission,0,1));
            } else colors[row*(nu+1)+col]=render_color(world,normal,color,renderer.material_emission+renderer.object_fill);
        }
        for (int row=0;row<nv;++row) for (int col=0;col<nu;++col) for (int k=0;k<4;++k) {
            int cx=col+(k==1 || k==2),cy=row+(k>=2);
            double s=cx/(double)nu,t=cy/(double)nv;
            Vec3 p=V((vertices[f][0][0]+s*(vertices[f][1][0]-vertices[f][0][0])+t*(vertices[f][3][0]-vertices[f][0][0]))*.5,
                     (vertices[f][0][1]+s*(vertices[f][1][1]-vertices[f][0][1])+t*(vertices[f][3][1]-vertices[f][0][1]))*.5,
                     (vertices[f][0][2]+s*(vertices[f][1][2]-vertices[f][0][2])+t*(vertices[f][3][2]-vertices[f][0][2]))*.5);
            Vec3 lit=colors[cy*(nu+1)+cx];
            glColor3d(lit.x,lit.y,lit.z); glTexCoord2d(s*u,t*v); glVertex3d(p.x,p.y,p.z);
        }
        free(colors);
    }
    glEnd();
    glPopMatrix();
}

void draw_sphere(Vec3 center,double radius,Vec3 base) {
    if (renderer.collecting_shadows) { lighting_add_sphere(center,radius); return; }
    enum { LAT=24,LON=48 };
    int lat=museum.unlit_mode ? 12 : LAT,lon=museum.unlit_mode ? 24 : LON;
    Vec3 colors[LAT+1][LON+1];
    for (int row=0;row<=lat;++row) for (int col=0;col<=lon;++col) {
        double a=-PI/2+PI*row/lat,b=2*PI*col/lon;
        Vec3 n=V(cos(a)*cos(b),sin(a),cos(a)*sin(b));
        Vec3 p=V(center.x+radius*n.x,center.y+radius*n.y,center.z+radius*n.z);
        colors[row][col]=render_color(p,n,base,.09);
    }
    material(base,T_NONE,32);
    for (int row=0;row<lat;++row) {
        glBegin(GL_TRIANGLE_STRIP);
        for (int col=0;col<=lon;++col) for (int k=0;k<2;++k) {
            int r=row+k;
            double a=-PI/2+PI*r/lat,b=2*PI*col/lon;
            Vec3 color=colors[r][col]; glColor3d(color.x,color.y,color.z);
            glVertex3d(center.x+radius*cos(a)*cos(b),center.y+radius*sin(a),center.z+radius*cos(a)*sin(b));
        }
        glEnd();
    }
}

int render_init(const char *executable) {
    make_texture(T_GRASS,106,128,91);
    make_texture(T_STONE,196,194,181);
    if (load_earth_texture(executable)) return 1;
    fputs("Falha ao carregar a textura da Terra.\n",stderr);
    glDeleteTextures(T_COUNT-1,&textures[1]);
    return 0;
}

void render_shutdown(void) {
    clear_render_cache();
    glDeleteTextures(T_COUNT-1,&textures[1]);
}

static void lit_vertex(Vec3 p,Vec3 normal,Vec3 base) {
    Vec3 color=museum.unlit_mode ? base : render_color(object_world(p,0),object_world(normal,1),base,.07);
    glColor3d(color.x,color.y,color.z); glVertex3d(p.x,p.y,p.z);
}

void ring_z(double inner,double outer,double depth,Vec3 base) {
    int segments=museum.unlit_mode ? 24 : 40;
    if (renderer.collecting_shadows) {
        if (inner==0) lighting_add_rolled_occluder(object_world(V(0,0,0),0),V(2*outer,2*outer,depth),object_roll);
        else for (int i=0;i<20;++i) {
            double a=(i+.5)*2*PI/20,r=(inner+outer)/2;
            lighting_add_rolled_occluder(object_world(V(r*cos(a),r*sin(a),0),0),
                V(2*r*sin(PI/20)*1.03,outer-inner,depth),object_roll+a*180/PI+90);
        }
        return;
    }
    material(base,T_NONE,32);
    glBegin(GL_QUADS);
    for (int i=0;i<segments;++i) {
        double a=i*2*PI/segments,b=(i+1)*2*PI/segments;
        for (int face=-1;face<=1;face+=2) {
            Vec3 n=V(0,0,face); double z=face*depth/2;
            lit_vertex(V(outer*cos(a),outer*sin(a),z),n,base);
            lit_vertex(V(outer*cos(b),outer*sin(b),z),n,base);
            lit_vertex(V(inner*cos(b),inner*sin(b),z),n,base);
            lit_vertex(V(inner*cos(a),inner*sin(a),z),n,base);
        }
        for (int edge=0;edge<2;++edge) {
            double r=edge ? inner : outer,sign=edge ? -1 : 1;
            if (r==0) continue;
            Vec3 na=V(sign*cos(a),sign*sin(a),0),nb=V(sign*cos(b),sign*sin(b),0);
            lit_vertex(V(r*cos(a),r*sin(a),-depth/2),na,base);
            lit_vertex(V(r*cos(b),r*sin(b),-depth/2),nb,base);
            lit_vertex(V(r*cos(b),r*sin(b),depth/2),nb,base);
            lit_vertex(V(r*cos(a),r*sin(a),depth/2),na,base);
        }
    }
    glEnd();
}
