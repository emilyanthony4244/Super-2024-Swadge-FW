#ifndef _PUZZLEDROP_MODE_H_
#define _PUZZLEDROP_MODE_H_


//
// 
//

#define PUZ_US_PER_FRAME 16667

#define NUM_FRAME_TIMES  60

#include "swadge2024.h"
#include "puzzledrop_tile.h"

extern swadgeMode_t puzzledropMode;

typedef struct
{
    wsg_t tileBackground;

    wsg_t orangeTiles;
    
    puzzledropTile_t* tiles[182];

    int8_t currentMode; 

    int16_t tile_y;

    font_t ibm_vga8;
    int32_t frameTimer;

    uint32_t frameTimes[NUM_FRAME_TIMES];
    uint32_t frameTimesIdx;
} puzzledropVars_t;

#endif