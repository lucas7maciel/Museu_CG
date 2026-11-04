/* Inicializacao da janela, opcoes e laco principal. */
#include "museum.h"
#include "building.h"
#include "camera.h"
#include "interaction.h"
#include "render.h"
#include "self_test.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void error_callback(int code,const char *description) { fprintf(stderr,"GLFW %d: %s\n",code,description); }

int main(int argc,char **argv) {
    int smoke=0;
    const char *capture=NULL;
    for (int i=1;i<argc;++i) {
        if (strcmp(argv[i],"--self-test")==0) return self_test();
        else if (strcmp(argv[i],"--smoke-test")==0) smoke=1;
        else if (strcmp(argv[i],"--unlit")==0 || strcmp(argv[i],"--no-lighting")==0) museum.unlit_mode=1;
        else if (strcmp(argv[i],"--inside")==0) { museum.position=V(0,FLOOR_Y+HEIGHT,17.8); museum.yaw=-90; museum.pitch=-10; }
        else if (strcmp(argv[i],"--corridor")==0) { museum.position=V(0,FLOOR_Y+HEIGHT,-.4); museum.yaw=-90; museum.pitch=-14; }
        else if (strcmp(argv[i],"--earth")==0) { museum.position=V(EARTH_X,FLOOR_Y+HEIGHT,EARTH_Z+3.4); museum.yaw=-90; museum.pitch=12; }
        else if (strcmp(argv[i],"--balance")==0) { museum.position=V(BALANCE_X,FLOOR_Y+HEIGHT,BALANCE_Z+3.7); museum.yaw=-90; museum.pitch=6; }
        else if (strcmp(argv[i],"--sculpture")==0) { museum.position=V(SCULPTURE_X-1,FLOOR_Y+HEIGHT,SCULPTURE_Z+5); museum.yaw=atan2(-5,1)*180/PI; museum.pitch=9; }
        else if (strcmp(argv[i],"--pendulum")==0) { museum.position=V(PENDULUM_X,FLOOR_Y+HEIGHT,PENDULUM_Z+3.7); museum.yaw=-90; museum.pitch=10; }
        else if (strcmp(argv[i],"--gears")==0) { museum.position=V(-4,FLOOR_Y+HEIGHT,-10.3); museum.yaw=-90; museum.pitch=10; }
        else if (strcmp(argv[i],"--colors")==0) { museum.position=V(RGB_X,FLOOR_Y+HEIGHT,-9); museum.yaw=-90; museum.pitch=3; }
        else if (strcmp(argv[i],"--sculpture-open")==0) { museum.exhibits.sculpture_open=1; ++museum.exhibits.revision; }
        else if (strcmp(argv[i],"--pendulum-pulled")==0) { museum.exhibits.pendulum_angle=.8; ++museum.exhibits.revision; }
        else if (strcmp(argv[i],"--gears-turned")==0) { museum.exhibits.gear_phase=25; ++museum.exhibits.revision; }
        else if (strcmp(argv[i],"--colors-red-only")==0) { museum.exhibits.rgb[1]=museum.exhibits.rgb[2]=0; ++museum.exhibits.revision; }
        else if (strcmp(argv[i],"--colors-moved")==0) { museum.exhibits.green_x=6.2; ++museum.exhibits.revision; }
        else if (strcmp(argv[i],"--balance-left-heavy")==0) { museum.balance.radius[0]=BALANCE_MAX_RADIUS; museum.balance.angle=balance_target_angle(&museum.balance); }
        else if (strcmp(argv[i],"--balance-right-heavy")==0) { museum.balance.radius[1]=BALANCE_MAX_RADIUS; museum.balance.angle=balance_target_angle(&museum.balance); }
        else if (strcmp(argv[i],"--earth-turned")==0) museum.earth_angle=100;
        else if (strcmp(argv[i],"--no-earth-light")==0) earth_light_on=0;
        else if (strcmp(argv[i],"--no-sun")==0) museum.sunlight_on=0;
        else if (strcmp(argv[i],"--no-ceiling")==0) ceiling_lights_on=0;
        else if (strcmp(argv[i],"--no-strips")==0) corridor_lights_on=0;
        else if (strcmp(argv[i],"--no-exhibit-lights")==0) exhibit_lights_on=0;
        else if (strcmp(argv[i],"--gate-open")==0) { museum.gate_open=1; museum.gate_angle=90; }
        else if (strcmp(argv[i],"--windows-closed")==0) {
            for (int j=0;j<WINDOW_COUNT;++j) { museum.windows[j].open=0; museum.windows[j].angle=0; }
        }
        else if (strcmp(argv[i],"--screenshot")==0 && i+1<argc) capture=argv[++i];
        else { fprintf(stderr,"Argumento invalido: %s\n",argv[i]); return EXIT_FAILURE; }
    }
    build_museum();
    glfwSetErrorCallback(error_callback);
#ifdef __linux__
    /* Uma superficie Wayland oculta pode nao ter framebuffer no WSLg.
       Capturas/testes usam X11 quando disponivel; a janela normal usa o padrao. */
    if ((smoke || capture) && getenv("DISPLAY")) glfwInitHint(GLFW_PLATFORM,GLFW_PLATFORM_X11);
#endif
    if (!glfwInit()) return EXIT_FAILURE;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,1);
    glfwWindowHint(GLFW_SAMPLES,museum.unlit_mode ? 0 : 4);
    if (smoke || capture) glfwWindowHint(GLFW_VISIBLE,GLFW_FALSE);
    GLFWwindow *window=glfwCreateWindow(museum.unlit_mode ? 960 : 1280,museum.unlit_mode ? 600 : 800,"Museu OpenGL",NULL,NULL);
    if (!window) { glfwTerminate(); return EXIT_FAILURE; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(smoke || capture ? 0 : 1);
    glfwSetKeyCallback(window,key_callback);
    glfwSetCursorPosCallback(window,mouse_callback);
    glfwSetMouseButtonCallback(window,mouse_button_callback);
    glfwSetWindowFocusCallback(window,focus_callback);
    glfwSetInputMode(window,GLFW_CURSOR,GLFW_CURSOR_DISABLED);
    glEnable(GL_DEPTH_TEST); glEnable(GL_MULTISAMPLE);
    glDisable(GL_LIGHTING); /* Cores calculadas com oclusao em lighting.c. */
    if (!render_init(argv[0])) {
        glfwDestroyWindow(window); glfwTerminate(); return EXIT_FAILURE;
    }
    double previous=glfwGetTime();
    int frames=0,result=EXIT_SUCCESS;
    int smoke_unlit=museum.unlit_mode;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        double now=glfwGetTime(),frame_dt=fmin(now-previous,.25),dt=fmin(frame_dt,.04);
        previous=now;
        keyboard_look(dt,
            (glfwGetKey(window,GLFW_KEY_RIGHT)==GLFW_PRESS)-(glfwGetKey(window,GLFW_KEY_LEFT)==GLFW_PRESS),
            (glfwGetKey(window,GLFW_KEY_UP)==GLFW_PRESS)-(glfwGetKey(window,GLFW_KEY_DOWN)==GLFW_PRESS));
        move_player(dt,(glfwGetKey(window,GLFW_KEY_W)==GLFW_PRESS)-(glfwGetKey(window,GLFW_KEY_S)==GLFW_PRESS),
                    (glfwGetKey(window,GLFW_KEY_D)==GLFW_PRESS)-(glfwGetKey(window,GLFW_KEY_A)==GLFW_PRESS),
                    glfwGetKey(window,GLFW_KEY_LEFT_SHIFT)==GLFW_PRESS);
        update_gate(dt);
        update_windows(dt);
        update_earth(dt);
        update_balance(frame_dt);
        if (!capture) update_exhibits(frame_dt);
        int w,h;
        glfwGetFramebufferSize(window,&w,&h);
        if (!w || !h) { glfwWaitEventsTimeout(.05); continue; }
        glViewport(0,0,w,h);
        glClearColor(.61f,.76f,.86f,1);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
        camera(w,h);
        LightingStats before=lighting_stats();
        scene();
        if (smoke && museum.unlit_mode) {
            LightingStats after=lighting_stats();
            if (after.samples!=before.samples || after.shadow_rays!=before.shadow_rays) {
                fputs("Falha: modo leve executou calculos de luz/sombra.\n",stderr); result=EXIT_FAILURE; break;
            }
        }
        crosshair(w,h);
        if (smoke || capture) {
            GLenum error=glGetError();
            if (error!=GL_NO_ERROR) { fprintf(stderr,"Erro OpenGL: %u\n",(unsigned)error); result=EXIT_FAILURE; break; }
            /* WSLg pode alocar o framebuffer oculto apenas apos a primeira troca. */
            if (capture && ++frames>=2) {
                if (!screenshot(capture,w,h)) { fputs("Falha ao salvar captura.\n",stderr); result=EXIT_FAILURE; }
                break;
            }
        }
        glfwSwapBuffers(window);
        if (smoke) {
            if (++frames>=6) {
                LightingStats stats=lighting_stats();
                printf("OK: seis quadros em primeira pessoa e obras sem erros OpenGL | amostras de luz: %zu | raios de sombra: %zu.\n",stats.samples,stats.shadow_rays);
                break;
            }
            smoke_test_step(frames,smoke_unlit);
        }
    }
    render_shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return result;
}
