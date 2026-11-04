/* Verificacoes de logica e animacoes do teste de renderizacao. */
#include "museum.h"
#include "building.h"
#include "camera.h"
#include "interaction.h"
#include "render_internal.h"
#include "self_test.h"
#include <stdio.h>
#include <stdlib.h>

static int window_self_test(void) {
#define W_CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA janela: %s\n",msg); return EXIT_FAILURE; } } while (0)
    for (int i=0;i<WINDOW_COUNT;++i) {
        Opening o=window_opening(i);
        double exterior=i<3 ? -1 : 1,cz=(o.start+o.end)/2;
        W_CHECK(museum.windows[i].open && museum.windows[i].angle==90,"estado inicial aberto");
        museum.position=V(exterior*11.2,HEIGHT,cz);
        museum.yaw=exterior>0 ? 180 : 0;
        museum.pitch=atan2(FLOOR_Y+(o.sill+o.head)/2-museum.position.y,3)*180/PI;
        W_CHECK(aimed_target()==i+1,"selecao pelo exterior");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        W_CHECK(!museum.windows[i].open && museum.gate_open,"clique fecha apenas a janela selecionada");
        for (int j=0;j<WINDOW_COUNT;++j) if (j!=i) W_CHECK(museum.windows[j].open,"outras janelas permanecem abertas");
        update_windows(.5);
        W_CHECK(museum.windows[i].angle==45,"animacao intermediaria de fechamento");
        Vec3 mid=window_world(V(0,1,window_width(i)/2),i,0);
        W_CHECK(window_collision(i,mid.x,mid.z,FLOOR_Y,HEIGHT,45),"colisao acompanha folha em rotacao");
        update_windows(.5);
        W_CHECK(museum.windows[i].angle==0,"fechamento completo");
        W_CHECK(window_collision(i,exterior*8.29,cz-.1,FLOOR_Y,HEIGHT,0),"folha fechada tem colisao");
        museum.position=V(exterior*5.2,FLOOR_Y+HEIGHT,cz); museum.pitch=0; museum.yaw=exterior>0 ? 0 : 180;
        W_CHECK(aimed_target()==i+1,"selecao pelo interior");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        update_windows(.5);
        W_CHECK(museum.windows[i].angle==45,"abertura pelo interior");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        update_windows(.5);
        W_CHECK(!museum.windows[i].open && museum.windows[i].angle==0,"clique inverte movimento em andamento");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        update_windows(1);
        W_CHECK(museum.windows[i].open && museum.windows[i].angle==90,"reabertura completa");
        W_CHECK(!window_collision(i,exterior*8.29,cz,FLOOR_Y,HEIGHT,90),"centro do vao fica livre");
        Vec3 open_mid=window_world(V(0,1,window_width(i)/2),i,0);
        W_CHECK(window_collision(i,open_mid.x,open_mid.z,FLOOR_Y,HEIGHT,90),"folha aberta mantem colisao");
        museum.position.x=exterior*16;
        museum.yaw=exterior>0 ? 180 : 0;
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        W_CHECK(museum.windows[i].open,"clique distante ignorado");
        museum.position.x=exterior*5.2; museum.yaw=exterior>0 ? 180 : 0;
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        W_CHECK(museum.windows[i].open,"clique olhando para outro lado ignorado");
    }
    Vec3 hinge=window_hinge(0,0);
    double middle=window_width(0)*.5/sqrt(2);
    museum.position=V(hinge.x-middle,HEIGHT,hinge.z+middle);
    museum.windows[0].open=0; update_windows(1);
    W_CHECK(museum.windows[0].blocked && museum.windows[0].angle>0 &&
            !window_collision(0,museum.position.x,museum.position.z,0,HEIGHT+.12,museum.windows[0].angle),"folha pausa antes de atingir jogador");
    museum.position=V(0,FLOOR_Y+HEIGHT,10); update_windows(1);
    W_CHECK(!museum.windows[0].blocked && museum.windows[0].angle==0,"animacao continua quando jogador se afasta");
    museum.windows[0].open=1; update_windows(1);
    Vec3 origin=V(-5.2,FLOOR_Y+HEIGHT,.5),target=window_world(V(0,1,window_width(0)*.8),0,0);
    Vec3 delta=V(target.x-origin.x,target.y-origin.y,target.z-origin.z);
    double length=sqrt(delta.x*delta.x+delta.y*delta.y+delta.z*delta.z);
    W_CHECK(interaction_target(origin,V(delta.x/length,delta.y/length,delta.z/length))==-1,"parede oculta janela e bloqueia clique");
    return EXIT_SUCCESS;
#undef W_CHECK
}

static int earth_self_test(void) {
#define E_CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA Terra: %s\n",msg); return EXIT_FAILURE; } } while (0)
    museum.earth_held=0;
    museum.position=V(EARTH_X,FLOOR_Y+HEIGHT,EARTH_Z+3.4); museum.yaw=-90;
    museum.pitch=atan2(EARTH_Y-museum.position.y,3.4)*180/PI;
    E_CHECK(aimed_target()==TARGET_EARTH,"raio seleciona a esfera visivel");
    double angle=museum.earth_angle;
    update_earth(.5);
    E_CHECK(museum.earth_angle==angle,"sem pressionar a esfera fica parada");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS,0);
    E_CHECK(!museum.earth_held,"botao direito nao inicia rotacao");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    E_CHECK(museum.earth_held,"pressionar sobre a esfera inicia rotacao");
    const int removed_keys[]={GLFW_KEY_ESCAPE,GLFW_KEY_V,GLFW_KEY_G};
    for (size_t i=0;i<COUNT(removed_keys);++i) {
        key_callback(NULL,removed_keys[i],0,GLFW_PRESS,0);
        E_CHECK(museum.earth_held && aimed_target()==TARGET_EARTH,"teclas removidas preservam interacao e mira em primeira pessoa");
    }
    collect_light_occluders();
    E_CHECK(lighting_visibility(V(0,1,10),V(0,1,0),100)==0,"teto permanece solido apos teclas removidas");
    update_earth(1);
    E_CHECK(fabs(museum.earth_angle-fmod(angle+45,360))<1e-6,"rotacao continua durante o pressionamento");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_RELEASE,0);
    angle=museum.earth_angle; update_earth(1);
    E_CHECK(!museum.earth_held && museum.earth_angle==angle,"soltar para imediatamente");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    focus_callback(NULL,0);
    E_CHECK(!museum.earth_held,"perder foco cancela pressionamento");
    museum.position.z=EARTH_Z+10;
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    E_CHECK(!museum.earth_held,"clique fora do alcance ignorado");
    E_CHECK(!walkable(EARTH_X,EARTH_Z,FLOOR_Y) && walkable(EARTH_X+2.2,EARTH_Z,FLOOR_Y),"colisao e espaco para circular");
    Vec3 origin=V(-9,EARTH_Y,13),delta=V(EARTH_X-origin.x,0,EARTH_Z-origin.z);
    double length=hypot(delta.x,delta.z);
    E_CHECK(interaction_target(origin,V(delta.x/length,0,delta.z/length))!=TARGET_EARTH,"parede bloqueia selecao da Terra");
    collect_light_occluders();
    Vec3 n=V(0,-.6,.8),p=V(EARTH_X,EARTH_Y+EARTH_RADIUS*n.y,EARTH_Z+EARTH_RADIUS*n.z);
    earth_light_on=1;
    Vec3 on=lighting_sample(p,n,V(1,1,1),0,0);
    earth_light_on=0;
    Vec3 off=lighting_sample(p,n,V(1,1,1),0,0);
    E_CHECK(on.z>off.z+.5,"luz do chao ilumina hemisferio inferior");
    earth_light_on=1;
    E_CHECK(lighting_visibility(V(EARTH_X,5.4,EARTH_Z),V(0,-1,0),4)==0,"esfera projeta sombra");
    museum.earth_angle=0;
    puts("OK: Terra, selecao, segurar/soltar, cancelamento, colisao, luz ascendente e sombra.");
    return EXIT_SUCCESS;
#undef E_CHECK
}

static int balance_interaction_test(void) {
#define B_CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA interacao balanca: %s\n",msg); return EXIT_FAILURE; } } while (0)
    B_CHECK(balance_self_test()==EXIT_SUCCESS,"ciclo e inclinacao");
    balance_reset(&museum.balance); museum.earth_held=0;
    for (int side=0;side<2;++side) {
        Vec3 ball=balance_ball_center(&museum.balance,side);
        museum.position=V(BALANCE_X,FLOOR_Y+HEIGHT,BALANCE_Z+4.1);
        museum.yaw=atan2(ball.z-museum.position.z,ball.x-museum.position.x)*180/PI;
        museum.pitch=atan2(ball.y-museum.position.y,hypot(ball.x-museum.position.x,ball.z-museum.position.z))*180/PI;
        int selected=aimed_target();
        B_CHECK(selected==TARGET_BALANCE_LEFT+side,"selecao independente das bolas");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        B_CHECK(museum.balance.held==side && !museum.earth_held,"pressionar seleciona somente esta bola");
        double before=museum.balance.radius[side],other=museum.balance.radius[1-side];
        update_balance(.5);
        B_CHECK(museum.balance.radius[side]>before && museum.balance.radius[1-side]==other,"somente bola pressionada cresce");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_RELEASE,0);
        before=museum.balance.radius[side]; update_balance(1);
        B_CHECK(museum.balance.held==-1 && museum.balance.radius[side]==before,"soltar para mudanca de tamanho");
    }
    B_CHECK(fabs(museum.balance.radius[0]-museum.balance.radius[1])<1e-7 && museum.balance.angle==0,"duas bolas iguais equilibram a balanca");
    museum.balance.held=0; focus_callback(NULL,0);
    B_CHECK(museum.balance.held==-1,"perder foco encerra pressionamento");
    museum.position.z=35; museum.balance.held=0; update_balance(.1);
    B_CHECK(museum.balance.held==-1,"afastamento cancela ciclo");
    B_CHECK(!walkable(BALANCE_X,BALANCE_Z,FLOOR_Y) && walkable(BALANCE_X,BALANCE_Z+2,FLOOR_Y),"colisao da base e passagem ao redor");
    balance_reset(&museum.balance);
    return EXIT_SUCCESS;
#undef B_CHECK
}

static void test_aim(Vec3 from,Vec3 target) {
    museum.position=from; museum.yaw=atan2(target.z-from.z,target.x-from.x)*180/PI;
    museum.pitch=atan2(target.y-from.y,hypot(target.x-from.x,target.z-from.z))*180/PI;
}

static int exhibits_interaction_test(void) {
#define X_CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA interacao obras: %s\n",msg); return EXIT_FAILURE; } } while (0)
    X_CHECK(exhibits_self_test()==EXIT_SUCCESS,"logica das quatro obras");
    for (int target=TARGET_SCULPTURE;target<=TARGET_RGB_LAMP;++target) {
        exhibits_reset(&museum.exhibits); museum.earth_held=0; museum.balance.held=-1;
        Vec3 from=V(RGB_X,FLOOR_Y+HEIGHT,-9),point;
        if (target==TARGET_SCULPTURE) { from=V(SCULPTURE_X,FLOOR_Y+HEIGHT,SCULPTURE_Z+3.8); point=sculpture_piece(&museum.exhibits,13); }
        else if (target==TARGET_PENDULUM) { from=V(PENDULUM_X,FLOOR_Y+HEIGHT,PENDULUM_Z+3.7); point=pendulum_bob(&museum.exhibits); }
        else if (target>=TARGET_GEAR_0 && target<=TARGET_GEAR_2) { from=V(-4,FLOOR_Y+HEIGHT,-10.3); point=gear_center(target-TARGET_GEAR_0); }
        else if (target>=TARGET_RGB_R && target<=TARGET_RGB_B) point=rgb_button(target-TARGET_RGB_R);
        else point=rgb_lamp(&museum.exhibits,1);
        test_aim(from,point);
        X_CHECK(aimed_target()==target,"selecao de cada obra e controle por raio");
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
        if (target>=TARGET_RGB_R && target<=TARGET_RGB_B) {
            X_CHECK(museum.exhibits.held==-1 && !museum.exhibits.rgb[target-TARGET_RGB_R],"clique alterna apenas canal escolhido");
            mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
            X_CHECK(museum.exhibits.rgb[target-TARGET_RGB_R],"segundo clique religa canal");
        } else {
            X_CHECK(museum.exhibits.held==target && !museum.earth_held && museum.balance.held==-1,"segurar aciona apenas obra selecionada");
            unsigned before=museum.exhibits.revision; update_exhibits(.25);
            X_CHECK(museum.exhibits.revision!=before,"obra responde enquanto pressionada");
            if (target==TARGET_PENDULUM) {
                double old_yaw=museum.yaw,angle=museum.exhibits.pendulum_angle;
                keyboard_look(.25,1,0);
                X_CHECK(museum.yaw==old_yaw && museum.exhibits.pendulum_angle>angle,"setas puxam pendulo sem girar camera");
            }
        }
        mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_RELEASE,0);
        X_CHECK(museum.exhibits.held==-1,"soltar termina a interacao");
    }
    exhibits_press(&museum.exhibits,TARGET_SCULPTURE); focus_callback(NULL,0);
    X_CHECK(museum.exhibits.held==-1,"perder foco cancela interacao");
    exhibits_press(&museum.exhibits,TARGET_RGB_LAMP); museum.position=V(0,HEIGHT,30); update_exhibits(.1);
    X_CHECK(museum.exhibits.held==-1,"distancia cancela interacao");
    X_CHECK(walkable(0,5,FLOOR_Y) && walkable(0,-4,FLOOR_Y) && walkable(0,-14,FLOOR_Y),"salas e corredor mantem passagem central");
    X_CHECK(!walkable(SCULPTURE_X,SCULPTURE_Z,FLOOR_Y) && !walkable(PENDULUM_X,PENDULUM_Z,FLOOR_Y) &&
        !walkable(-4.1,-15,FLOOR_Y) && !walkable(RGB_X,-10.7,FLOOR_Y),"colisoes das quatro instalacoes");
    key_callback(NULL,GLFW_KEY_R,0,GLFW_PRESS,0);
    X_CHECK(museum.exhibits.sculpture_open==0 && museum.exhibits.pendulum_angle==0 && museum.exhibits.gear_phase==0 &&
        museum.exhibits.rgb[0] && museum.exhibits.rgb[1] && museum.exhibits.rgb[2],"R restaura as obras");
    return EXIT_SUCCESS;
#undef X_CHECK
}

int self_test(void) {
#define CHECK(c, msg) do { if (!(c)) { fprintf(stderr,"FALHA: %s\n",msg); return EXIT_FAILURE; } } while (0)
    build_museum();
    CHECK(fabs(ground_height(0,10)-FLOOR_Y)<1e-6 && ground_height(0,-4)==FLOOR_Y && ground_height(0,-15)==FLOOR_Y,"piso conectado dos dois saloes e corredor");
    CHECK(ground_height(5,-4)==0,"recuos externos livres");
    CHECK(!wall_collision(0,20,FLOOR_Y,HEIGHT),"vao de entrada livre de paredes");
    CHECK(!walkable(0,20,FLOOR_Y),"portao fechado bloqueia entrada");
    CHECK(!walkable(4,20,FLOOR_Y),"parede frontal");
    for (int side=0;side<2;++side) {
        double x=side ? 8.2 : -8.2;
        for (size_t i=0;i<3;++i) {
            Opening opening=window_opening(side*3+(int)i);
            double z=(opening.start+opening.end)/2;
            CHECK(!walkable(x,z,FLOOR_Y),"peitoril impede passagem");
            CHECK(!wall_collision(x,z,FLOOR_Y+1.1,1.7),"vao de janela realmente vazio");
            CHECK(wall_collision(x,z,FLOOR_Y+3.7,.2),"verga solida acima da janela");
        }
    }
    CHECK(!walkable(5,20.1,0),"nao subir pela lateral da plataforma");
    CHECK(!walkable(2.1,-4,FLOOR_Y),"colisao lateral do corredor");
    CHECK(!walkable(0,-20,FLOOR_Y),"parede posterior");
    CHECK(!walkable(38,0,0),"limite do terreno");
    museum.position=V(0,HEIGHT,29); museum.pitch=85;
    move_player(1,1,0,0);
    CHECK(fabs(museum.position.x)<1e-6 && fabs(museum.position.z-26)<1e-6 && museum.position.y==HEIGHT,"movimento horizontal independente do pitch");
    museum.position=V(0,HEIGHT,29);
    move_player(1,1,1,0);
    CHECK(fabs(hypot(museum.position.x,museum.position.z-29)-3)<1e-6,"diagonal normalizada");
    museum.position=V(0,FLOOR_Y+HEIGHT,24.5); museum.pitch=0;
    CHECK(aimed_target()==0,"mira reconhece portao proximo");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_RIGHT,GLFW_PRESS,0);
    CHECK(!museum.gate_open,"botao direito nao aciona portao");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_RELEASE,0);
    CHECK(!museum.gate_open,"soltar botao nao aciona portao");
    museum.position.z=29;
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    CHECK(!museum.gate_open,"clique distante ignorado");
    museum.position.z=24.5; museum.yaw=90;
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    CHECK(!museum.gate_open,"clique olhando para outro lado ignorado");
    museum.yaw=-90;
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    CHECK(museum.gate_open,"clique esquerdo abre portao");
    update_gate(.5);
    CHECK(fabs(museum.gate_angle-45)<1e-6,"abertura animada intermediaria");
    double mid=GATE_WIDTH*.5/sqrt(2);
    CHECK(gate_collision(-GATE_HALF+mid,GATE_Z-mid,FLOOR_Y,HEIGHT,museum.gate_angle),"colisao acompanha folha girada");
    update_gate(.5);
    CHECK(museum.gate_angle==90 && walkable(0,20,FLOOR_Y),"portao aberto libera entrada");
    CHECK(gate_collision(-GATE_HALF,19,FLOOR_Y,HEIGHT,museum.gate_angle) &&
          gate_collision(GATE_HALF,19,FLOOR_Y,HEIGHT,museum.gate_angle),"ambas as folhas abertas mantem colisao");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    CHECK(!museum.gate_open,"clique no vao aberto fecha portao");
    update_gate(1);
    CHECK(museum.gate_angle==0 && !walkable(0,20,FLOOR_Y),"fechamento completo restaura colisao");
    museum.position=V(0,FLOOR_Y+HEIGHT,16); museum.yaw=90;
    CHECK(aimed_target()==0,"interacao pelo interior");
    mouse_button_callback(NULL,GLFW_MOUSE_BUTTON_LEFT,GLFW_PRESS,0);
    update_gate(1);
    CHECK(museum.gate_angle==90,"abertura pelo interior");
    museum.position=V(-GATE_HALF+mid,FLOOR_Y+HEIGHT,GATE_Z-mid);
    museum.gate_open=0; update_gate(1);
    CHECK(museum.gate_blocked && museum.gate_angle>0 && !gate_collision(museum.position.x,museum.position.z,FLOOR_Y,HEIGHT+.12,museum.gate_angle),"fechamento pausa antes de atingir jogador");
    museum.position=V(0,HEIGHT,25); update_gate(1);
    CHECK(museum.gate_angle==0 && !museum.gate_blocked,"fechamento continua apos desocupar arco");
    museum.gate_open=1; update_gate(1); museum.yaw=-90;
    museum.position=V(0,HEIGHT,25);
    for (int i=0;i<370;++i) move_player(.04,1,0,0);
    CHECK(museum.position.z < -18 && museum.position.z > -19.7 && fabs(museum.position.y-(FLOOR_Y+HEIGHT))<1e-6,"subir escada e atravessar os dois saloes");
    for (int i=0;i<380;++i) move_player(.04,-1,0,0);
    CHECK(museum.position.z>25 && fabs(museum.position.y-HEIGHT)<1e-6,"descer escada ate o terreno");
    CHECK(window_self_test()==EXIT_SUCCESS,"seis janelas interativas");
    CHECK(lighting_self_test()==EXIT_SUCCESS,"calculo das fontes e sombras");
    collect_light_occluders();
    Vec3 sun=lighting_sun_direction(),sun_patch=V(-6,FLOOR_Y+.018,3.5);
    CHECK(lighting_visibility(sun_patch,sun,100)==1,"sol entra pela janela aberta");
    museum.windows[0].angle=0; museum.windows[0].open=0; collect_light_occluders();
    CHECK(lighting_visibility(sun_patch,sun,100)==0,"fechar a janela bloqueia o sol");
    CHECK(lighting_visibility(V(-6,FLOOR_Y+.018,9),sun,100)==1,"outra janela aberta continua recebendo sol");
    CHECK(lighting_visibility(V(0,1,10),V(0,1,0),100)==0,"teto bloqueia luz externa");
    Vec3 floor_color=lighting_sample(V(-1.6,FLOOR_Y,-4),V(0,1,0),V(1,1,1),0,0);
    corridor_lights_on=0;
    Vec3 no_strip=lighting_sample(V(-1.6,FLOOR_Y,-4),V(0,1,0),V(1,1,1),0,0);
    CHECK(floor_color.x>no_strip.x+.1 && floor_color.x>floor_color.z,"faixa quente ilumina piso do corredor");
    int strip_regions=0;
    for (size_t i=0;i<museum_light_count;++i) if (museum_lights[i].type==LIGHT_STRIP) {
        const MuseumLight *strip=&museum_lights[i];
        for (int k=1;k<8;++k) {
            double t=k/8.0,x=strip->position.x+(strip->end.x-strip->position.x)*t;
            double z=strip->position.z+(strip->end.z-strip->position.z)*t;
            CHECK(ground_height(x,z)==FLOOR_Y,"rodape permanece no interior do museu");
            CHECK(wall_collision(x-strip->mounting_normal.x*.18,z-strip->mounting_normal.z*.18,FLOOR_Y,.4),"faixa acompanha uma parede");
            CHECK(!(z>19.5 && fabs(x)<2.35),"rodape nao atravessa a entrada");
            strip_regions|=z>0 ? 1 : z>-7.6 ? 2 : 4;
        }
    }
    CHECK(strip_regions==7,"rodapes presentes nos dois saloes e no corredor");
    corridor_lights_on=1; museum.windows[0].angle=90; museum.windows[0].open=1;
    CHECK(earth_self_test()==EXIT_SUCCESS,"exposicao da Terra");
    CHECK(balance_interaction_test()==EXIT_SUCCESS,"balanca interativa");
    CHECK(exhibits_interaction_test()==EXIT_SUCCESS,"quatro novas obras interativas");
    puts("OK: planta, navegacao, portao e seis janelas: cliques, animacao, oclusao e colisoes.");
    return EXIT_SUCCESS;
#undef CHECK
}

void smoke_test_step(int frames,int smoke_unlit) {
    /* O teste normal verifica tambem a ida/volta por F2 com caches novos. */
    if (!smoke_unlit && (frames==2 || frames==3)) key_callback(NULL,GLFW_KEY_F2,0,GLFW_PRESS,0);
    museum.position=frames<3 ? V(0,FLOOR_Y+HEIGHT,18) : V(0,FLOOR_Y+HEIGHT,-8.5);
    museum.yaw=-90; museum.pitch=-8;
    museum.sunlight_on=(frames/2)%2;
    ceiling_lights_on=frames%2; corridor_lights_on=(frames/2)%2;
    exhibit_lights_on=frames%2;
    museum.gate_open=frames<3;
    update_gate(.5);
    for (int i=0;i<WINDOW_COUNT;++i) museum.windows[i].open=(frames+i)%3==0;
    update_windows(.5);
    earth_light_on=frames%2;
    museum.earth_angle=frames*45;
    museum.balance.radius[0]=frames%3==0 ? BALANCE_MAX_RADIUS : BALANCE_INITIAL_RADIUS;
    museum.balance.radius[1]=frames%3==1 ? BALANCE_MAX_RADIUS : BALANCE_INITIAL_RADIUS;
    museum.balance.angle=balance_target_angle(&museum.balance);
    museum.exhibits.sculpture_open=(frames%3)/2.0; museum.exhibits.pendulum_angle=(frames%3-1)*.55;
    museum.exhibits.gear_phase=frames*20; museum.exhibits.rgb[frames%3]=!museum.exhibits.rgb[frames%3];
    museum.exhibits.green_x=3+frames*.5; ++museum.exhibits.revision;
}
