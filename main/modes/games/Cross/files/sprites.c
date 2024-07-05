#include "sprites.h"

wsg_t CrossIdle1;
wsg_t CrossIdle2;
wsg_t CrossWalk1;
wsg_t CrossWalk2;
wsg_t CrossWalk3;
wsg_t CrossWalk4;
wsg_t CrossWalk5;
wsg_t CrossSpin1;
wsg_t CrossSpin2;
wsg_t CrossSpin3;
wsg_t CrossSpin4;
wsg_t CrossSpin5;
wsg_t CrossSpin6;
wsg_t CrossJump;
wsg_t CrossFall;

void loadSprites() {
    loadWsg("CrossIdle1.wsg", &CrossIdle1, true);
    loadWsg("CrossIdle2.wsg", &CrossIdle2, true);
    loadWsg("CrossWalk1.wsg", &CrossWalk1, true);
    loadWsg("CrossWalk2.wsg", &CrossWalk2, true);
    loadWsg("CrossWalk3.wsg", &CrossWalk3, true);
    loadWsg("CrossWalk4.wsg", &CrossWalk4, true);
    loadWsg("CrossWalk5.wsg", &CrossWalk5, true);
    loadWsg("CrossSpin1.wsg", &CrossSpin1, true);
    loadWsg("CrossSpin2.wsg", &CrossSpin2, true);
    loadWsg("CrossSpin3.wsg", &CrossSpin3, true);
    loadWsg("CrossSpin4.wsg", &CrossSpin4, true);
    loadWsg("CrossSpin5.wsg", &CrossSpin5, true);
    loadWsg("CrossSpin6.wsg", &CrossSpin6, true);
    loadWsg("CrossJump.wsg", &CrossJump, true);
    loadWsg("CrossFall.wsg", &CrossFall, true);
}

void deloadSprites(){
    freeWsg(&CrossIdle1);
    freeWsg(&CrossIdle2);
    freeWsg(&CrossWalk1);
    freeWsg(&CrossWalk2);
    freeWsg(&CrossWalk3);
    freeWsg(&CrossWalk4);
    freeWsg(&CrossWalk5);
    freeWsg(&CrossSpin1);
    freeWsg(&CrossSpin2);
    freeWsg(&CrossSpin3);
    freeWsg(&CrossSpin4);
    freeWsg(&CrossSpin5);
    freeWsg(&CrossSpin6);
    freeWsg(&CrossJump);
    freeWsg(&CrossFall);
}

void drawSprite(int frame,int sprite,int spX1,int spY1, bool direction, Player* p){
    if(sprite != 2){
        p->dash = 1;
    }
    if(sprite == 0){
        if(frame == 0){ drawWsg(&CrossIdle1, spX1, spY1, direction, false, 0); } 
        else { drawWsg(&CrossIdle2, spX1, spY1, direction, false, 0); }
    }
    else if(sprite == 1){
        if(frame == 0){ drawWsg(&CrossWalk1, spX1, spY1, direction, false, 0); } 
        else if (frame == 1) { drawWsg(&CrossWalk2, spX1, spY1, direction, false, 0); }
        else if (frame == 2) { drawWsg(&CrossWalk3, spX1, spY1, direction, false, 0); }
        else if (frame == 3) { drawWsg(&CrossWalk4, spX1, spY1, direction, false, 0); }
        else if (frame == 4) { drawWsg(&CrossWalk5, spX1, spY1, direction, false, 0); }
    }
    else if(sprite == 2){
        if(frame == 0){ drawWsg(&CrossSpin1, spX1, spY1, direction, false, 0); } 
        else if (frame == 1) { drawWsg(&CrossSpin2, spX1, spY1, direction, false, 0); }
        else if (frame == 2) { drawWsg(&CrossSpin3, spX1, spY1, direction, false, 0); }
        else if (frame == 3) { drawWsg(&CrossSpin4, spX1, spY1, direction, false, 0); }
        else if (frame == 4) { drawWsg(&CrossSpin5, spX1, spY1, direction, false, 0); }
        else if (frame == 5) { drawWsg(&CrossSpin6, spX1, spY1, direction, false, 0); }
    }
    else if(sprite == 3){
        drawWsg(&CrossJump, spX1, spY1, direction, false, 0);
    }
    else if(sprite == 4){
        drawWsg(&CrossFall, spX1, spY1, direction, false, 0);
    }
}