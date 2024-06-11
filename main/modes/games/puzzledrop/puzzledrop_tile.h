#pragma once

#include "mode_puzzledrop.h"

typedef struct 
{
    int8_t color; // 0 - Orange 1 - Red
    int16_t x;
    int16_t y;
    bool active;
    bool dropping;
}puzzledropTile_t;
