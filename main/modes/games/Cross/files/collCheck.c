#include "collCheck.h"
#include <stdio.h>

int check_collision(int pos[2], int pw, int ph, int collider[4]) {
    int player_left = pos[0];
    int player_right = pos[0] + pw;
    int player_top = pos[1];
    int player_bottom = pos[1] + ph;

    int collider_left = collider[0];
    int collider_right = collider[0] + collider[2];
    int collider_top = collider[1];
    int collider_bottom = collider[1] + collider[3];

    if (player_bottom < collider_top ||
        player_top > collider_bottom ||
        player_right < collider_left ||
        player_left > collider_right) {
        return NO_COLLISION;
    }

    if (player_bottom >= collider_top &&
        player_top < collider_top &&
        player_right > collider_left &&
        player_left < collider_right) {
        return COLLISION_FEET;
    }

    if (player_top <= collider_bottom &&
        player_bottom > collider_bottom &&
        player_right > collider_left &&
        player_left < collider_right) {
        return COLLISION_HEAD;
    }

    if (player_right > collider_left && player_left < collider_right) {
        if (player_left < collider_left && player_right > collider_left) {
            return COLLISION_LEFT_SIDE;
        }
        if (player_right > collider_right && player_left < collider_right) {
            return COLLISION_RIGHT_SIDE;
        }
    }

    return NO_COLLISION;
}