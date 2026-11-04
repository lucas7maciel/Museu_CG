#include "balance.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define PI 3.14159265358979323846

void balance_reset(BalanceState *s) {
    s->radius[0]=s->radius[1]=BALANCE_INITIAL_RADIUS;
    s->growing[0]=s->growing[1]=1;
    s->angle=0; s->held=-1;
}

double balance_target_angle(const BalanceState *s) {
    /* Mesmo material e bracos iguais: o peso e proporcional ao volume. */
    double left=s->radius[0],right=s->radius[1];
    double mass_difference=left*left*left-right*right*right;
    double maximum=pow(BALANCE_MAX_RADIUS,3)-pow(BALANCE_MIN_RADIUS,3);
    if (fabs(left-right)<1e-7) return 0;
    return BALANCE_MAX_TILT*mass_difference/maximum;
}

void balance_update(BalanceState *s,double dt) {
    if (dt<=0) return;
    if (s->held>=0 && s->held<2) {
        int i=s->held;
        double range=BALANCE_MAX_RADIUS-BALANCE_MIN_RADIUS;
        double phase=s->radius[i]-BALANCE_MIN_RADIUS;
        if (!s->growing[i]) phase=2*range-phase;
        /* Onda triangular: preserva sobras de tempo ao atingir cada limite. */
        phase=fmod(phase+BALANCE_GROW_SPEED*dt,2*range);
        if (phase<range) { s->radius[i]=BALANCE_MIN_RADIUS+phase; s->growing[i]=1; }
        else { s->radius[i]=BALANCE_MAX_RADIUS-(phase-range); s->growing[i]=0; }
    }
    double target=balance_target_angle(s);
    s->angle+=(target-s->angle)*(1-exp(-6*dt));
    if (fabs(target-s->angle)<.002) s->angle=target;
}

Vec3 balance_endpoint(const BalanceState *s,int side) {
    double sign=side ? 1 : -1,a=s->angle*PI/180;
    return (Vec3){BALANCE_X+sign*BALANCE_ARM*cos(a),BALANCE_PIVOT_Y+sign*BALANCE_ARM*sin(a),BALANCE_Z};
}

Vec3 balance_ball_center(const BalanceState *s,int side) {
    Vec3 center=balance_endpoint(s,side);
    center.y+=-BALANCE_DROP+.05+s->radius[side];
    return center;
}

int balance_self_test(void) {
#define CHECK(c,msg) do { if (!(c)) { fprintf(stderr,"FALHA balanca: %s\n",msg); return EXIT_FAILURE; } } while (0)
    BalanceState s; balance_reset(&s);
    CHECK(s.angle==0 && balance_target_angle(&s)==0,"tamanhos iguais ficam equilibrados");
    balance_update(&s,1);
    CHECK(s.radius[0]==BALANCE_INITIAL_RADIUS && s.radius[1]==BALANCE_INITIAL_RADIUS,"sem pressionar nao muda tamanho");
    s.held=0; balance_update(&s,.5);
    CHECK(s.radius[0]>s.radius[1] && s.angle>0 && balance_ball_center(&s,0).y<balance_ball_center(&s,1).y,"bola esquerda maior desce");
    s.radius[0]=BALANCE_MAX_RADIUS-.01; s.growing[0]=1; balance_update(&s,.1);
    CHECK(!s.growing[0] && s.radius[0]<BALANCE_MAX_RADIUS && s.radius[0]>BALANCE_MAX_RADIUS-.03,"inverte ao atingir limite superior");
    s.radius[0]=BALANCE_MIN_RADIUS+.01; s.growing[0]=0; balance_update(&s,.1);
    CHECK(s.growing[0] && s.radius[0]>BALANCE_MIN_RADIUS,"inverte ao atingir limite inferior");
    double frozen=s.radius[0]; s.held=-1; balance_update(&s,1);
    CHECK(s.radius[0]==frozen,"soltar preserva tamanho");
    s.held=1; balance_update(&s,100);
    CHECK(s.radius[1]>=BALANCE_MIN_RADIUS && s.radius[1]<=BALANCE_MAX_RADIUS && s.radius[0]==frozen,"bola direita independente e limites com dt grande");
    s.held=-1; s.radius[0]=BALANCE_MIN_RADIUS; s.radius[1]=BALANCE_MAX_RADIUS; balance_update(&s,5);
    CHECK(s.angle<0 && balance_endpoint(&s,1).y<balance_endpoint(&s,0).y,"bola direita maior desce");
    s.radius[0]=s.radius[1]; balance_update(&s,5);
    CHECK(s.angle==0 && balance_endpoint(&s,0).y==balance_endpoint(&s,1).y,"retorna suavemente ao equilibrio");
    puts("OK: balanca, ciclos independentes, limites, pausa, inclinacao e equilibrio.");
    return EXIT_SUCCESS;
#undef CHECK
}
