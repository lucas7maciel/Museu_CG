/* Materiais procedurais repetiveis e carregamento da textura terrestre. */
#include "textures.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TEXTURE_SIZE 256
static GLuint textures[T_COUNT];

static double noise(int x,int y,unsigned seed) {
    uint32_t hash=(uint32_t)x*374761393u+(uint32_t)y*668265263u+seed*1442695041u;
    hash=(hash^(hash>>13))*1274126177u;
    hash^=hash>>16;
    return (hash&65535u)/32767.5-1;
}

static Vec3 surface_pixel(int id,int x,int y) {
    double u=x/(double)TEXTURE_SIZE,v=y/(double)TEXTURE_SIZE;
    double n=noise(x,y,(unsigned)id),shade;
    if (id==T_FLOOR) {
        int plank=x/64,across=x%64,along=(y+(plank%2)*64)%128;
        /* Tabuas desencontradas, variacao por tabua e veios longitudinais. */
        double grain=sin(2*PI*(96*u+.45*sin(4*PI*v)+.2*sin(12*PI*v)));
        shade=5*grain+3*n+7*noise(plank,(y+(plank%2)*64)/128,23);
        if (across<2 || along<2) return V(131+n*2,115+n*2,91+n*2);
        if (across==2 || along==2) shade-=9;
        return V(216+shade,197+shade*.9,166+shade*.75);
    }
    if (id==T_WALL) {
        /* Reboco pintado: grao discreto, sem juntas nem sombras gravadas. */
        shade=2*n+1.5*sin(8*PI*u)*sin(6*PI*v);
        if (noise(x,y,37)>.97) shade-=4;
        return V(240+shade,237+shade,230+shade);
    }
    if (id==T_CEILING) {
        int dx=x%16-8,dy=y%16-8;
        shade=n*1.5;
        if (x%128<2 || y%128<2) shade-=31;
        else if (dx*dx+dy*dy<3) shade-=30;
        return V(232+shade,230+shade,222+shade);
    }
    if (id==T_STONE) {
        shade=4*n+2*sin(6*PI*u+sin(4*PI*v));
        if (x<2 || y<2 || x>=TEXTURE_SIZE-2 || y>=TEXTURE_SIZE-2) shade-=28;
        return V(215+shade,213+shade,201+shade);
    }
    shade=7*n+3*sin(36*PI*u+sin(16*PI*v));
    return V(106+shade,128+shade,91+shade*.8);
}

static int upload_texture(int id,int width,int height,const unsigned char *pixels,int repeat_t) {
    glGenTextures(1,&textures[id]);
    if (!textures[id]) return 0;
    glBindTexture(GL_TEXTURE_2D,textures[id]);
    glPixelStorei(GL_UNPACK_ALIGNMENT,1);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_S,GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_WRAP_T,repeat_t ? GL_REPEAT : GL_CLAMP_TO_EDGE);
    return gluBuild2DMipmaps(GL_TEXTURE_2D,GL_RGB,width,height,GL_RGB,GL_UNSIGNED_BYTE,pixels)==0;
}

static int make_surface(int id) {
    unsigned char pixels[TEXTURE_SIZE*TEXTURE_SIZE*3];
    for (int y=0;y<TEXTURE_SIZE;++y) for (int x=0;x<TEXTURE_SIZE;++x) {
        Vec3 color=surface_pixel(id,x,y);
        size_t offset=((size_t)y*TEXTURE_SIZE+(size_t)x)*3;
        pixels[offset]=(unsigned char)clamp(color.x,0,255);
        pixels[offset+1]=(unsigned char)clamp(color.y,0,255);
        pixels[offset+2]=(unsigned char)clamp(color.z,0,255);
    }
    return upload_texture(id,TEXTURE_SIZE,TEXTURE_SIZE,pixels,1);
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
    if (ok) ok=upload_texture(T_EARTH,width,height,pixels,0);
    free(pixels);
    return ok;
}

int textures_init(const char *executable) {
    const int surfaces[]={T_GRASS,T_STONE,T_FLOOR,T_WALL,T_CEILING};
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_MODULATE);
    for (size_t i=0;i<COUNT(surfaces);++i) if (!make_surface(surfaces[i])) {
        textures_shutdown();
        return 0;
    }
    if (load_earth_texture(executable)) return 1;
    textures_shutdown();
    return 0;
}

void textures_shutdown(void) {
    glDeleteTextures(T_COUNT-1,&textures[1]);
    memset(textures,0,sizeof(textures));
}

GLuint texture_handle(int id) {
    return textures[id];
}

void texture_coordinates(int id,Vec3 point,Vec3 normal,double *u,double *v) {
    if (fabs(normal.y)>.5) { *u=point.x; *v=point.z; }
    else if (fabs(normal.x)>.5) { *u=point.z; *v=point.y; }
    else { *u=point.x; *v=point.y; }
    double width=2,length=2;
    if (id==T_FLOOR) { width=1.28; length=5.12; }
    else if (id==T_STONE) width=length=1;
    else if (id==T_CEILING) width=length=1.2;
    *u/=width; *v/=length;
}
