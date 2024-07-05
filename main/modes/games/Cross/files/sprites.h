#ifndef SPRITES_H
#define SPRITES_H

#include "spiffs_wsg.h"
#include "CrossPlayer.h"
#include <stdbool.h>

extern wsg_t CrossIdle1;
extern wsg_t CrossIdle2;
extern wsg_t CrossWalk1;
extern wsg_t CrossWalk2;
extern wsg_t CrossWalk3;
extern wsg_t CrossWalk4;
extern wsg_t CrossWalk5;
extern wsg_t CrossSpin1;
extern wsg_t CrossSpin2;
extern wsg_t CrossSpin3;
extern wsg_t CrossSpin4;
extern wsg_t CrossSpin5;
extern wsg_t CrossSpin6;
extern wsg_t CrossJump;
extern wsg_t CrossFall;

void loadSprites();
void deloadSprites();
void drawSprite(int frame,int sprite,int spX1,int spY1, bool direction, Player* p);

#endif // SPRITES_H
