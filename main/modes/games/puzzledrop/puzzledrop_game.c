//==============================================================================
// Includes
//==============================================================================
#define NUM_COLUMNS          7
#define NUM_ROWS            13
#define CELL_SIZE           16

#define OFFSET_X            60
#define OFFSET_Y            20

#include <esp_log.h>

#include "puzzledrop_game.h"

void puzzledropDraw(puzzledropVars_t* p)
{
    
    for (int x = 0; x < NUM_COLUMNS; x++)
    {
        for (int y = 0; y < NUM_ROWS; y++)
            drawWsgSimple(&p->tileBackground, OFFSET_X + (x * CELL_SIZE), OFFSET_Y + (y * CELL_SIZE) );
    }
    drawLine(OFFSET_X + (CELL_SIZE * NUM_COLUMNS) - 1, OFFSET_Y, OFFSET_X + (CELL_SIZE * NUM_COLUMNS) -1, OFFSET_Y + (CELL_SIZE * NUM_ROWS) -1, c555, 0);
    drawLine(OFFSET_X, OFFSET_Y, OFFSET_X, OFFSET_Y + (CELL_SIZE * NUM_ROWS) -1, c555, 0);
    drawLine(OFFSET_X, OFFSET_Y , OFFSET_X + (CELL_SIZE * NUM_COLUMNS) - 1, OFFSET_Y, c555, 0);
    drawLine(OFFSET_X, OFFSET_Y + (CELL_SIZE * NUM_ROWS) -1, OFFSET_X + (CELL_SIZE * NUM_COLUMNS) - 1, OFFSET_Y + (CELL_SIZE * NUM_ROWS) -1, c555, 0);
    
    // Calculate and draw FPS
    int32_t startIdx  = (p->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    uint32_t tElapsed = p->frameTimes[p->frameTimesIdx] - p->frameTimes[startIdx];
    if (0 != tElapsed)
    {
        uint32_t fps = (1000000 * NUM_FRAME_TIMES) / tElapsed;

        char tmp[16];
        snprintf(tmp, sizeof(tmp) - 1, "%" PRIu32, fps);
        drawText(&p->ibm_vga8, c555, tmp, 135, 2);
    }

        char tmp[8];
        snprintf(tmp, sizeof(tmp) - 1, "%" PRIu32, p->tile_y);
        drawText(&p->ibm_vga8, c555, tmp, 135, 16);

    for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
    {
        if (!p->tiles[i]->active || p->tiles[i]->x < 0 || p->tiles[i]->y < 0) continue;
        puzzledropTile_t* tile = p->tiles[i];


        drawWsgSimple(&p->orangeTiles, tile->x + OFFSET_X, tile->y + OFFSET_Y);

    }
    
}

#define STATE_DROPPING     1
#define STATE_CHECKING     2

void puzzledropUpdate(puzzledropVars_t* p)
{
    if (p->currentMode == STATE_DROPPING)
    {
        p->tile_y += 4;
        if (p->tile_y >= 212 - OFFSET_Y)
            p->tile_y = 212- OFFSET_Y;


        for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
        {
            if (!p->tiles[i]->dropping || !p->tiles[i]->active) continue;

            puzzledropTile_t* tile = p->tiles[i];


            tile->y += 1;

            if (tile->y >= 212 - OFFSET_Y)
            {
                tile->y = 212 - OFFSET_Y;
                tile->dropping = false;
            }
        }
    }
}