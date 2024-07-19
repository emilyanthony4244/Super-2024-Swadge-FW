//==============================================================================
// Includes
//==============================================================================
#define NUM_COLUMNS          7
#define NUM_ROWS            13
#define CELL_SIZE           16

#define OFFSET_X            60
#define OFFSET_Y            20


//Squongy!
#define STATE_CONTROL      1
#define STATE_DROPPING     2
#define STATE_CHECKING     3
#define STATE_NEWOBJECT    4
#define STATE_GAMEOVER     5

#define COLOR_NONE         0
#define COLOR_ORANGE       1
#define COLOR_RED          2
#define COLOR_BLUE         3

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
        //drawText(&p->ibm_vga8, c555, tmp, 135, 16);

    for (int i = 0; i < NUM_COLUMNS * NUM_ROWS; i++)
    {
        if (i >= ARRAY_SIZE(p->board)) continue;


        if (i == p->tile1)
        {
            //tickOffset
            drawWsgSimple(&p->orangeTiles, ((i % NUM_COLUMNS) * CELL_SIZE ) + OFFSET_X, ((i / NUM_COLUMNS)  * CELL_SIZE ) + OFFSET_Y + p->tickOffset);
        }
        else if (i == p->tile2)
        {
            drawWsgSimple(&p->orangeTiles, ((i % NUM_COLUMNS) * CELL_SIZE ) + OFFSET_X, ((i / NUM_COLUMNS)  * CELL_SIZE ) + OFFSET_Y + p->tickOffset);
        }
        else if (p->board[i] != 0)
        {
            drawWsgSimple(&p->orangeTiles, ((i % NUM_COLUMNS) * CELL_SIZE ) + OFFSET_X, ((i / NUM_COLUMNS)  * CELL_SIZE ) + OFFSET_Y);
        }

        
    }


    for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
    {
        if (!p->tiles[i]->active || p->tiles[i]->x < 0 || p->tiles[i]->y < 0) continue;
        puzzledropTile_t* tile = p->tiles[i];


        drawWsgSimple(&p->orangeTiles, tile->x + OFFSET_X, tile->y + OFFSET_Y);

    }
    
    ////////
    puzzleStartLevel(p);
    //p->gameSpeed = 8;
    p->controlTickCnt = 0;
}

void puzzleStartLevel(puzzledropVars_t* p)
{
    p->gameSpeed = 4;
    //p->tickOffset = 0;
    p->controlTickCnt = 0;
}

void puzzledropUpdate(puzzledropVars_t* p, int64_t elapsedUs)
{
    if (p->currentMode == STATE_CONTROL)
    {
        int tile1Destination = p->tile1;
        int tile2Destination = p->tile2;

        p->controlTickCnt -= elapsedUs/ 1000;
        bool update = p->controlTickCnt < 0;
        printf("Test %d %d\n", p->tickOffset, p->controlTickCnt);
        if (p->controlTickCnt < 0)
        {
            p->controlTickCnt = 128;
            p->tickOffset++;
            if (p->tickOffset >= CELL_SIZE)
            {
                p->tickOffset -= CELL_SIZE;
                ESP_LOGI("PUZZ_GAME", "Tick");
                
                p->board[p->tile1] = 0;
                p->board[p->tile2] = 0;
                p->tile1 += NUM_COLUMNS;
                p->tile2 += NUM_COLUMNS;
            }
        }

        p->board[p->tile1] = 0;
        p->board[p->tile2] = 0;


        //if (moving/transitioning)
        /*
           if left is pressed p->tile1 --;
           if right is pressed p->tile1 ++;
        */

        for (int i = 96; i > 16; i--)
        {
            //p->board[i];
        }

        p->board[p->tile1] = 1;
        p->board[p->tile2] = 1;

    }

}

void puzzledropNewObject(puzzledropVars_t* p, int64_t elapsedUs)
{
    p->tile1 = 3;
    p->tile2 = 10;
    p->currentMode = STATE_CONTROL;
}

void puzzledropUpdate_bk(puzzledropVars_t* p, int64_t elapsedUs)
{
    if (p->currentMode == STATE_DROPPING)
    {
        bool collision = false;
        int collisionOffset;
        int tileDestination;
        int destinationTileX = 0;
        int destinationTileY = 0;

        // for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
        p->controlTickCnt -= elapsedUs/ 1000;
        bool update = p->controlTickCnt < 0;
        if (p->controlTickCnt < 0)
        {
            p->controlTickCnt = 128;
            //printf("Tick %d", elapsedUs);
        }

        
        //Do it from bottom up ???
        for (int i =  ARRAY_SIZE(p->tiles) -1; i >= 0; i--)
        {
            if (!p->tiles[i]->dropping || !p->tiles[i]->active) continue;

            puzzledropTile_t* tile = p->tiles[i];

            if (update)
            {
                tile->dy += p->gameSpeed;
            }

            destinationTileX = tile->dx/CELL_SIZE;
            destinationTileY = ((tile->dy + CELL_SIZE + p->gameSpeed)/CELL_SIZE);
            
            if (tile->dy < OFFSET_Y)
            {
                if (p->board[destinationTileX + (destinationTileY * NUM_COLUMNS)] != 0)
                {
                    printf("\n\nGame over!");
                    collision = false;
                    p->currentMode = STATE_GAMEOVER;
                    break;
                }
            }
            else if (tile->dy >= 212 - OFFSET_Y)
            {
                destinationTileY = NUM_ROWS -1;
                tile->dy = 212 - OFFSET_Y;
                tile->dropping = false;
                collision = true;
                p->board[destinationTileX + (destinationTileY * NUM_COLUMNS)] = tile->color;
                printf("Bottom out! %d %d\n", destinationTileX + (destinationTileY * NUM_COLUMNS), tile->dy/CELL_SIZE);
            }
            else
            {

                if (destinationTileY < NUM_ROWS && p->board[destinationTileX + (destinationTileY * NUM_COLUMNS)] != 0)
                {
                    tile->dy = (destinationTileY - 1) * CELL_SIZE;
                    tile->dropping = false;
                    collision = true;
                    p->board[destinationTileX + ((destinationTileY -1) * NUM_COLUMNS)] =  tile->color;
                    printf("Got here! %d,%d index->%d color->%d \n",  destinationTileX, destinationTileY, destinationTileX + (destinationTileY * NUM_COLUMNS), p->board[destinationTileX + (destinationTileY * NUM_COLUMNS)]);
                }
                
            }
        }
       


        if (p->currentMode == STATE_GAMEOVER)
        {
            //Can't get here!
            printf("Got here?");
            for (int i =  ARRAY_SIZE(p->tiles) -1; i >= 0; i--)
            {
                if (!p->tiles[i]->dropping || !p->tiles[i]->active) continue;

                puzzledropTile_t* tile = p->tiles[i];
                tile->active = false;
                //tile->dx = tile->x / CELL_SIZE;
                tile->dy = -100;
            }
        }

        
        for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
        {
            if (!p->tiles[i]->active) continue;

            puzzledropTile_t* tile = p->tiles[i];

            tile->y = tile->dy;
            tile->x = tile->dx;
        }



        if (collision)
        {
            p->currentMode = STATE_CHECKING;
        }
        
    }


}

void puzzledropCheck(puzzledropVars_t* p)
{
    int xx = ARRAY_SIZE(p->board);
    printf("TT BOOM! %d \n---\n", xx);

    for (int i = 0; i < ARRAY_SIZE(p->board); i++)
    {
        if (i % NUM_COLUMNS == 0)
        {
            printf("\n");
        }
        printf("%d ", p->board[i]);
    }
    printf("\n----");
    
    printf("New Color check %d \n", COLOR_ORANGE);
    for (int x = 0; x < 2; x++)
    {

        for (int i = 0; i < ARRAY_SIZE(p->tiles); i++)
        {
            if (p->tiles[i]->active) continue;

            p->tiles[i]->active = true;
            p->tiles[i]->dx = 3 * CELL_SIZE;
            p->tiles[i]->dy = -x * CELL_SIZE;
            p->tiles[i]->dropping = true;
            p->tiles[i]->color = COLOR_ORANGE;

            break;
        }
    }

    //if game not over
    p->currentMode = STATE_DROPPING;
}