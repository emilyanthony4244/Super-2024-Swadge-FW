#ifndef CROSSPLAYER_H
#define CROSSPLAYER_H

#include <stdlib.h>

typedef struct {
    int positions[2];
    int camera[2];
    int cheat;
    int lastPos[2];
    float speed[2];
    int falling;
    int dash;
    int anim;
} Player;

void initPlayer(Player* plyer, int x, int y, int camX, int camY);
void movePlayer(Player* plyer, int h, int states[3]);
void camUpdater(Player* plyer, int h);

#endif // CROSSPLAYER_H
