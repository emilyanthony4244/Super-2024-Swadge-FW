#pragma once

#include "mode_puzzledrop.h"

typedef struct 
{
    int8_t color; // 0 - Orange 1 - Red
    int16_t px;
    int16_t py;
    int16_t x;
    int16_t y;
    int16_t dx;
    int16_t dy;
    bool active;
    bool dropping;
}puzzledropTile_t;


typedef struct
{
    int m; //[104]int
    int cm;  //[104]int
    int score;
    int chainCnt; 
    int level;
    int count;
}puzzledropBoard_t;