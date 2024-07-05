#ifndef COLLCHECK_H
#define COLLCHECK_H

#include <stdbool.h>
#include "CrossPlayer.h"

#define NO_COLLISION 0
#define COLLISION_FEET 1
#define COLLISION_HEAD 2
#define COLLISION_LEFT_SIDE 3
#define COLLISION_RIGHT_SIDE 4

int check_collision(int pos[2], int pw, int ph, int collider[4]);

#endif