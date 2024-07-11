#include "CrossPlayer.h"
#include <stdio.h>

void initPlayer(Player* plyer, int x, int y, int camX, int camY) {
    plyer->positions[0] = x;
    plyer->positions[1] = y;
    plyer->camera[0] = camX;
    plyer->camera[1] = camY;
    plyer->cheat = 1;
    plyer->lastPos[0] = 0;
    plyer->lastPos[1] = 0;
    plyer->speed[0] = 0;
    plyer->speed[1] = 0;
    plyer->falling = 0;
    plyer->dash = 0;
    plyer->anim = 0;
}

void movePlayer(Player* plyer, int h, int states[3]) {
    plyer->lastPos[0] = plyer->positions[0];
    plyer->lastPos[1] = plyer->positions[1];

    if (plyer->cheat == 1) {
        plyer->anim = 0;
        if (states[2] == 1 && plyer->falling < 2) {
            plyer->positions[1] -= 1;
            plyer->speed[1] = -3.5;
            plyer->speed[0] *= 1.5;
        }
        if (states[0] == 1) {
            plyer->speed[0] -= 0.7;
            plyer->anim = 1;
        }
        if (states[0] == 2) {
            plyer->speed[0] += 0.7;
            plyer->anim = 1;
        }

        plyer->speed[1] += 0.3;
        plyer->speed[0] *= 0.9;

        if (plyer->speed[1] > 6) {
            plyer->speed[1] = 6;
        }

        plyer->falling += 1;

        if (plyer->positions[1] >= (40 - h)) {
            plyer->speed[1] = 0;
            plyer->positions[1] = (40 - h);
            plyer->falling = 0;
        } else {
            if (plyer->speed[1] > 0) {
                plyer->anim = 4;
            } else if (plyer->speed[1] < 0) {
                plyer->anim = 3;
            }
        }

        if ((states[2] == 2 && plyer->dash == 1) && (states[1] != 0 || states[0] != 0)) {
            if (states[1] == 2) {
                plyer->speed[1] = -4;
            }
            if (states[1] == 1) {
                plyer->speed[1] = 4;
            }
            if (states[0] == 2) {
                plyer->speed[0] = 8;
            }
            if (states[0] == 1) {
                plyer->speed[0] = -8;
            }
            plyer->anim = 2;
            plyer->dash = 0;
        }
    } else {
        if (states[0] == 1) {
            plyer->speed[0] -= 0.7;
        }
        if (states[0] == 2) {
            plyer->speed[0] += 0.7;
        }
        if (states[1] == 2) {
            plyer->speed[1] = -3.5;
        }
        if (states[1] == 1) {
            plyer->speed[1] += 0.7;
        }

        plyer->speed[0] *= 0.8;
        plyer->speed[1] *= 0.8;
    }

    /*
    if (states[2] == 2) {
        plyer->cheat += 1;

        if (plyer->cheat >= 2){
            plyer->cheat = 0;
        }
    }
    */

    plyer->positions[0] += (int)plyer->speed[0];
    plyer->positions[1] += (int)plyer->speed[1];

    printf("Player position: (%d, %d)\n", plyer->positions[0], plyer->positions[1]);
}

void camUpdater(Player* plyer, int h) {
    int x_center_min = 100, x_center_max = 180;
    int y_center_min = 80, y_center_max = 160;

    int distance_x = abs((plyer->positions[0] + plyer->camera[0]) - (x_center_min + x_center_max) / 2);
    int distance_y = abs((plyer->positions[1] + plyer->camera[1]) - (y_center_min + y_center_max) / 2);

    int speed_multiplier_x = 1 + (int)(distance_x * 0.1);
    int speed_multiplier_y = 1 + (int)(distance_y * 0.1);

    if ((plyer->positions[0] + plyer->camera[0]) > x_center_max) {
        plyer->camera[0] -= speed_multiplier_x;
    } else if ((plyer->positions[0] + plyer->camera[0]) < x_center_min) {
        plyer->camera[0] += speed_multiplier_x;
    }

    if ((plyer->positions[1] + (h - 5) + plyer->camera[1]) < y_center_min) {
        plyer->camera[1] += speed_multiplier_y;
    } else if ((plyer->positions[1] + (h - 5) + plyer->camera[1]) > y_center_max) {
        plyer->camera[1] -= speed_multiplier_y;
    }

    printf("Camera position: (%d, %d)\n", plyer->camera[0], plyer->camera[1]);
}
