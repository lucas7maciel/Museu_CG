/* Estado inicial da sessao do museu. */
#include "museum.h"

MuseumState museum={
    .position={0,HEIGHT,29},
    .yaw=-90,.pitch=-5,.sunlight_on=1,
    .windows={{90,1,0},{90,1,0},{90,1,0},{90,1,0},{90,1,0},{90,1,0}},
    .balance={{BALANCE_INITIAL_RADIUS,BALANCE_INITIAL_RADIUS},0,{1,1},-1},
    .exhibits={.green_x=4.5,.green_direction=1,.rgb={1,1,1},.held=-1}
};
