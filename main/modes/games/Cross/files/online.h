#ifndef ONLINE_H
#define ONLINE_H

#include "CrossPlayer.h"

typedef struct {
    int otherPos[2];
    int otherAnim;
} OnlineStuff;

int sendData(OnlineStuff* os, int x, int y, int anim, int chunk[10]);
int recieveData(OnlineStuff* os);

#endif