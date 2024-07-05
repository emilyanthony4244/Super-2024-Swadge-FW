#include "Cross.h"
#include "files/CrossPlayer.h"
#include "files/collCheck.h"
#include <math.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

// Function prototypes
static void crossEnterMode();
static void crossExitMode();
void ZaWarudoFix();

static void crossMainLoop(int64_t elapsedUs);
void renderWorld(Player* p, int lastAnim);
static void animate(Player* p, int lastAnim);
static void btns();
static void handle_collision(Player* p);
void lastState(Player* p);

static const char crossName[] = "Cross";

swadgeMode_t CrossMode = {
    .modeName = crossName,
    .wifiMode = ESP_NOW,
    .overrideUsb = false,
    .usesAccelerometer = false,
    .usesThermometer = false,
    .overrideSelectBtn = false,
    .fnMainLoop = crossMainLoop,
    .fnEnterMode = crossEnterMode,
    .fnExitMode = crossExitMode
};

typedef struct {
    int w_h[2];
    int states[3];
    bool debug;
    int count;
    int counter;
    int frame;
    int frameLimit;
    int lastAnim;
    int lockLimit;
    bool direction;
} CrossP_t;

CrossP_t* CrossP = NULL;
Player plyr;
int fpss = 1000000 / 30;
uint16_t btnStatez;
bool runOnceCuzWhyNot = true;

static void crossEnterMode() {
    CrossP = calloc(1, sizeof(CrossP_t));
    if (CrossP == NULL) {
        return;
    }
    initPlayer(&plyr, 110, 150, 110, 150);
    CrossP->w_h[0] = 13;
    CrossP->w_h[1] = 13;
    CrossP->count = 0;
    CrossP->counter = 0;
    CrossP->frame = 0;
    CrossP->frameLimit = 0;
    CrossP->debug = false;
    CrossP->lastAnim = 0;
    CrossP->lockLimit = 0;
    CrossP->direction = false;
}

static void crossExitMode() {
    deloadSprites();
    free(CrossP);
    CrossP = NULL;
}

int ZaWarudo[11][4] = {
    {20, 20, 30, 5}, {40, 22, 10, 18}, {-10, 30, 60, 5},
    {60, 30, 5, 5}, {65, 25, 5, 5}, {70, 20, 5, 5}, 
    {75, 15, 5, 5}, {80, 10, 5, 5}, {85, 5, 5, 5}, {90, 0, 5, 5},
    {-50, 10, 20, 30}
};

void renderWorld(Player* p, int lastAnimate) {
    fillDisplayArea(0, 0, 280, 240, c000);
    for (int i = 0; i < sizeof(ZaWarudo) / sizeof(ZaWarudo[0]); i++) {
        int* collidr = ZaWarudo[i];
        
        int x1 = collidr[0] + p->camera[0];
        int y1 = collidr[1] + p->camera[1];
        int x2 = x1 + collidr[2];
        int y2 = y1;
        drawLineFast(x1, y1, x2, y2, c555);

        x1 = x2;
        y1 = y2;
        y2 = y1 + collidr[3];
        drawLineFast(x1, y1, x2, y2, c555);

        x2 = collidr[0] + p->camera[0];
        y1 = y2;
        drawLineFast(x1, y1, x2, y2, c555);

        x1 = x2;
        y2 = collidr[1] + p->camera[1];
        drawLineFast(x1, y1, x2, y2, c555);
    }

    printf("Rendering player at (%d, %d) with size (%d, %d)\n", p->positions[0] + p->camera[0], p->positions[1] + p->camera[1], CrossP->w_h[0], CrossP->w_h[1]);
    
    if(CrossP->debug){
        int x1 = p->positions[0] + p->camera[0];
        int y1 = p->positions[1] + p->camera[1];
        int x2 = x1 + CrossP->w_h[0];
        int y2 = y1;
        drawLineFast(x1, y1, x2, y2, c555);

        x1 = x2;
        y1 = y2;
        y2 = y1 + CrossP->w_h[1];
        drawLineFast(x1, y1, x2, y2, c555);

        x2 = p->positions[0] + p->camera[0];
        y1 = y2;
        drawLineFast(x1, y1, x2, y2, c555);

        x1 = x2;
        y2 = p->positions[1] + p->camera[1];
        drawLineFast(x1, y1, x2, y2, c555);
    }

    animate(p, lastAnimate);
    int spX1 = p->positions[0] + p->camera[0];
    int spY1 = p->positions[1] + p->camera[1];
    drawSprite(CrossP->frame, p->anim, spX1, spY1, CrossP->direction, p);

    drawLineFast(0, 40 + p->camera[1], 280, 40 + p->camera[1], c555);
}

static void animate(Player* p, int la){
    if(CrossP->lockLimit >= 1){
        p->anim = la;
    }
    if(p->anim == la && CrossP->frameLimit != 0){
        if (CrossP->counter >= CrossP->count){
            CrossP->frame += 1;
            CrossP->counter = 0;
            if(CrossP->frame >= CrossP->frameLimit){
                CrossP->frame = 0;
                CrossP->lockLimit -= 1;
            }
        }
        CrossP->counter += 1;
    } else { 
        if ((p->anim == 2 || la == 2) && CrossP->lockLimit >= 1) {
            return;
        }

        CrossP->lockLimit = 0;

        CrossP->counter = 0;
        CrossP->frame = 0;

        if(p->anim == 0){
            CrossP->count = 4;
            CrossP->frameLimit = 2;
            CrossP->lockLimit = 0;
        }
        if(p->anim == 1){
            CrossP->count = 4;
            CrossP->frameLimit = 5;
            CrossP->lockLimit = 0;
        }
        if(p->anim == 2){
            CrossP->count = 2;
            CrossP->frameLimit = 6;
            CrossP->lockLimit = 2;
        }
        if(p->anim == 3){
            CrossP->count = 4;
            CrossP->frameLimit = 0;
            CrossP->lockLimit = 0;
        }
        if(p->anim == 4){
            CrossP->count = 4;
            CrossP->frameLimit = 0;
            CrossP->lockLimit = 0;
        }
    }
}

static void btns() {
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt)) {
        btnStatez = evt.state;
        printf("Button event: state: %04X, button: %d, down: %s\n",
            evt.state, evt.button, evt.down ? "down" : "up");
    }
    
    CrossP->states[0] = 0;
    CrossP->states[1] = 0;
    CrossP->states[2] = 0;

    if(btnStatez & PB_LEFT){
        CrossP->direction = true;
        CrossP->states[0] = 1;
    }
    if(btnStatez & PB_RIGHT){
        CrossP->direction = false;
        CrossP->states[0] = 2;
    }
    if(btnStatez & PB_DOWN){
        CrossP->states[1] = 1;
    }
    if(btnStatez & PB_UP){
        CrossP->states[1] = 2;
    }
    if(btnStatez & PB_A){
        CrossP->states[2] = 1;
    }
    if(btnStatez & PB_B){
        CrossP->states[2] = 2;
    }
}

#include "Cross.h"
#include <stdio.h>

#include "Cross.h"
#include <stdio.h>

static void handle_collision(Player* p) {
    int pw = CrossP->w_h[0];
    int ph = CrossP->w_h[1];

    for (int i = 0; i < sizeof(ZaWarudo) / sizeof(ZaWarudo[0]); i++) {
        int* collider = ZaWarudo[i];
        int colliding = check_collision(p->positions, pw, ph, (int[]){collider[0], collider[1], collider[2], collider[3]});
        
        if (colliding != NO_COLLISION) {
            if (colliding == COLLISION_FEET) {
                p->positions[1] = collider[1] - ph;
                p->speed[1] = 1;
                p->falling = 0;
                if (!(btnStatez & PB_LEFT) && !(btnStatez & PB_RIGHT)) {
                    p->anim = 0;
                } else {
                    p->anim = 1;
                }
            } else if (colliding == COLLISION_HEAD) {
                p->positions[1] = collider[1] + collider[3];
                p->speed[1] = 0;
            }
            if (colliding == COLLISION_LEFT_SIDE) {
                p->positions[0] = collider[0] - pw - 0.1;
                p->speed[0] = 0;
            } else if (colliding == COLLISION_RIGHT_SIDE) {
                p->positions[0] = collider[0] + collider[2] + 0.1;
                p->speed[0] = 0;
            }
        }
    }
}

void lastState(Player* p){
    CrossP->lastAnim = p->anim;
}

static void crossMainLoop(int64_t elapsedUs) {
    setFrameRateUs(fpss);

    if(runOnceCuzWhyNot){
        loadSprites();
    }runOnceCuzWhyNot = false;

    lastState(&plyr);
   
    btns();
    movePlayer(&plyr, CrossP->w_h[1], CrossP->states);
    camUpdater(&plyr, CrossP->w_h[1]);
    handle_collision(&plyr);

    renderWorld(&plyr, CrossP->lastAnim);
}