/* Montagem da cena, oclusores, caches, mira e capturas. */
#include "museum.h"
#include "building.h"
#include "interaction.h"
#include "render_internal.h"
#include <stdio.h>
#include <stdlib.h>

static GLuint static_scene_list,exhibits_scene_list;

void collect_light_occluders(void) {
    lighting_clear_occluders(); renderer.collecting_shadows=1;
    lighting_add_sphere(V(EARTH_X,EARTH_Y,EARTH_Z),EARTH_RADIUS);
    static_geometry(); draw_gate(); draw_windows();
    draw_balance();
    draw_exhibit_static(); draw_exhibit_dynamic();
    /* Batentes vedam as pequenas folgas entre folhas totalmente fechadas. */
    for (int i=0;i<WINDOW_COUNT;++i) if (museum.windows[i].angle<.1) {
        Opening o=window_opening(i);
        lighting_add_occluder(V((i<3 ? -1 : 1)*8.29,FLOOR_Y+(o.sill+o.head)/2,(o.start+o.end)/2),
            V(WINDOW_T,o.head-o.sill,o.end-o.start),0);
    }
    renderer.collecting_shadows=0;
}

static void draw_light_fixtures(void) {
    for (size_t i=0;i<museum_light_count;++i) {
        const MuseumLight *light=&museum_lights[i];
        if (light->type==LIGHT_SPOT || light->type==LIGHT_EXHIBIT_SPOT) {
            Vec3 p=light->position;
            double top=p.y+.26;
            box(V(p.x,(top+ROOF_Y)/2,p.z),V(.14,ROOF_Y-top,.14),V(.12,.14,.15),T_NONE,16);
            box(V(p.x,p.y+.18,p.z),V(.94,.16,.94),V(.12,.14,.15),T_NONE,16);
            int active=light->type==LIGHT_SPOT ? ceiling_lights_on : exhibit_lights_on;
            renderer.material_emission=active ? .95 : 0;
            box(V(p.x,p.y+.08,p.z),V(.76,.035,.76),light->color,T_NONE,8);
        } else if (light->type==LIGHT_STRIP) {
            Vec3 p=V((light->position.x+light->end.x)/2,(light->position.y+light->end.y)/2,(light->position.z+light->end.z)/2);
            Vec3 n=light->mounting_normal;
            double dx=fabs(light->end.x-light->position.x),dz=fabs(light->end.z-light->position.z);
            box(V(p.x-n.x*.055,p.y,p.z-n.z*.055),V(dx>0 ? dx+.10 : .09,.14,dz>0 ? dz+.10 : .09),V(.12,.12,.11),T_NONE,8);
            renderer.material_emission=corridor_lights_on ? .95 : 0;
            box(p,V(dx>0 ? dx : .025,.065,dz>0 ? dz : .025),light->color,T_NONE,8);
        } else {
            Vec3 p=light->position;
            box(V(p.x,FLOOR_Y+.035,p.z),V(1,.07,1),V(.1,.13,.15),T_NONE,8);
            renderer.material_emission=earth_light_on ? .95 : 0;
            box(V(p.x,FLOOR_Y+.078,p.z),V(.8,.025,.8),light->color,T_NONE,8);
        }
        renderer.material_emission=0;
    }
}

void clear_render_cache(void) {
    if (static_scene_list) glDeleteLists(static_scene_list,1);
    if (renderer.earth_scene_list) glDeleteLists(renderer.earth_scene_list,1);
    if (exhibits_scene_list) glDeleteLists(exhibits_scene_list,1);
    static_scene_list=renderer.earth_scene_list=exhibits_scene_list=0;
}

static void scene_unlit(void) {
    static int old_sun=-1,old_ceiling=-1,old_corridor=-1,old_earth=-1,old_exhibit=-1,old_rgb[3]={-1,-1,-1};
    static unsigned old_revision=(unsigned)-1;
    int dirty=!static_scene_list || old_sun!=museum.sunlight_on || old_ceiling!=ceiling_lights_on ||
        old_corridor!=corridor_lights_on || old_earth!=earth_light_on || old_exhibit!=exhibit_lights_on;
    for (int i=0;i<3;++i) if (old_rgb[i]!=museum.exhibits.rgb[i]) dirty=1;
    if (dirty) {
        if (!static_scene_list) static_scene_list=glGenLists(1);
        if (static_scene_list) glNewList(static_scene_list,GL_COMPILE);
        static_geometry(); draw_light_fixtures(); draw_exhibit_static();
        if (static_scene_list) glEndList();
        old_sun=museum.sunlight_on; old_ceiling=ceiling_lights_on;
        old_corridor=corridor_lights_on; old_earth=earth_light_on;
        old_exhibit=exhibit_lights_on;
        for (int i=0;i<3;++i) old_rgb[i]=museum.exhibits.rgb[i];
    }
    if (static_scene_list) glCallList(static_scene_list);
    else if (!dirty) { static_geometry(); draw_light_fixtures(); draw_exhibit_static(); }
    /* Animacoes nao invalidam o cenario e nao constroem oclusores neste modo. */
    draw_gate(); draw_windows(); draw_balance();
    if (!renderer.earth_scene_list) build_earth_mesh();
    draw_earth();
    if (!exhibits_scene_list || old_revision!=museum.exhibits.revision || dirty) {
        if (!exhibits_scene_list) exhibits_scene_list=glGenLists(1);
        if (exhibits_scene_list) glNewList(exhibits_scene_list,GL_COMPILE);
        draw_exhibit_dynamic();
        if (exhibits_scene_list) glEndList();
        old_revision=museum.exhibits.revision;
    }
    if (exhibits_scene_list) glCallList(exhibits_scene_list);
}

void scene(void) {
    static int old_unlit=-1;
    if (old_unlit!=museum.unlit_mode) {
        clear_render_cache();
        if (museum.unlit_mode) glDisable(GL_MULTISAMPLE); else glEnable(GL_MULTISAMPLE);
        old_unlit=museum.unlit_mode;
    }
    if (museum.unlit_mode) { scene_unlit(); return; }
    static double old_gate=-1,old_angles[WINDOW_COUNT];
    static int old_sun=-1,old_ceiling=-1,old_corridor=-1,old_earth=-1,old_exhibit=-1;
    static double old_balance_angle=999,old_radius[2];
    static unsigned shadow_revision=(unsigned)-1,static_revision=(unsigned)-1,draw_revision=(unsigned)-1;
    static double last_build=-1;
    static int old_rgb[3]={-1,-1,-1};
    int dirty=!static_scene_list || old_gate!=museum.gate_angle || old_sun!=museum.sunlight_on ||
        old_ceiling!=ceiling_lights_on || old_corridor!=corridor_lights_on || old_earth!=earth_light_on || old_exhibit!=exhibit_lights_on ||
        old_balance_angle!=museum.balance.angle || old_radius[0]!=museum.balance.radius[0] || old_radius[1]!=museum.balance.radius[1];
    for (int i=0;i<WINDOW_COUNT;++i) if (old_angles[i]!=museum.windows[i].angle) dirty=1;
    for (int i=0;i<3;++i) if (old_rgb[i]!=museum.exhibits.rgb[i]) dirty=1;
    if (dirty || shadow_revision!=museum.exhibits.revision) {
        lighting_set_rgb(museum.exhibits.rgb,museum.exhibits.green_x);
        collect_light_occluders();
        shadow_revision=museum.exhibits.revision;
    }
    double now=glfwGetTime();
    int rebuild=dirty || (static_revision!=museum.exhibits.revision && now-last_build>=.12);
    if (rebuild) {
        build_earth_mesh();
        if (!static_scene_list) static_scene_list=glGenLists(1);
        if (static_scene_list) glNewList(static_scene_list,GL_COMPILE);
        static_geometry(); draw_light_fixtures(); draw_balance(); draw_exhibit_static();
        if (static_scene_list) glEndList();
        old_gate=museum.gate_angle; old_sun=museum.sunlight_on;
        old_ceiling=ceiling_lights_on; old_corridor=corridor_lights_on;
        old_earth=earth_light_on;
        old_exhibit=exhibit_lights_on;
        old_balance_angle=museum.balance.angle; old_radius[0]=museum.balance.radius[0]; old_radius[1]=museum.balance.radius[1];
        for (int i=0;i<WINDOW_COUNT;++i) old_angles[i]=museum.windows[i].angle;
        for (int i=0;i<3;++i) old_rgb[i]=museum.exhibits.rgb[i];
        static_revision=museum.exhibits.revision; last_build=now;
    }
    if (static_scene_list) glCallList(static_scene_list);
    else if (!rebuild) { static_geometry(); draw_light_fixtures(); draw_balance(); draw_exhibit_static(); }
    draw_gate(); draw_windows();
    draw_earth();
    if (rebuild || draw_revision!=museum.exhibits.revision) {
        if (!exhibits_scene_list) exhibits_scene_list=glGenLists(1);
        if (exhibits_scene_list) glNewList(exhibits_scene_list,GL_COMPILE);
        draw_exhibit_dynamic();
        if (exhibits_scene_list) glEndList();
        draw_revision=museum.exhibits.revision;
    }
    if (exhibits_scene_list) glCallList(exhibits_scene_list);
}

void crosshair(int w,int h) {
    glPushAttrib(GL_ENABLE_BIT|GL_CURRENT_BIT|GL_LINE_BIT);
    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D); glDisable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity(); glOrtho(0,w,0,h,-1,1);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    double x=w*.5,y=h*.5;
    for (int pass=0;pass<2;++pass) {
        glLineWidth(pass ? 2 : 4);
        if (!pass) glColor3d(.1,.1,.1);
        else if (museum.earth_held || museum.balance.held>=0 || museum.exhibits.held>=0 || aimed_target()>=0) glColor3d(1,.78,.25);
        else glColor3d(.95,.95,.95);
        glBegin(GL_LINES);
        glVertex2d(x-8,y); glVertex2d(x-3,y); glVertex2d(x+3,y); glVertex2d(x+8,y);
        glVertex2d(x,y-8); glVertex2d(x,y-3); glVertex2d(x,y+3); glVertex2d(x,y+8);
        glEnd();
    }
    glPopMatrix(); glMatrixMode(GL_PROJECTION); glPopMatrix(); glMatrixMode(GL_MODELVIEW);
    glPopAttrib();
}

int screenshot(const char *path,int w,int h) {
    size_t stride=(size_t)w*3;
    unsigned char *pixels=malloc(stride*(size_t)h);
    if (!pixels) return 0;
    glPixelStorei(GL_PACK_ALIGNMENT,1);
    glReadBuffer(GL_BACK);
    glReadPixels(0,0,w,h,GL_RGB,GL_UNSIGNED_BYTE,pixels);
    FILE *f=fopen(path,"wb");
    if (!f) { free(pixels); return 0; }
    int ok=fprintf(f,"P6\n%d %d\n255\n",w,h)>0;
    for (int y=h-1;y>=0;--y) if (fwrite(pixels+(size_t)y*stride,1,stride,f)!=stride) ok=0;
    if (fclose(f)!=0) ok=0;
    free(pixels);
    return ok;
}
