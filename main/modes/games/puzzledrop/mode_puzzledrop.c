
#include <stdlib.h>

#include "mainMenu.h"
#include "menu.h"

#include "mode_puzzledrop.h"
#include "puzzledrop_game.h"

static const char puzzledropName[] = "Puzzle Drop";

static void puzzledropEnterMode(void);
static void puzzledropExitMode(void);
static void puzzledropMainLoop(int64_t elapsedUs);
static void puzzledropAudioCallback(uint16_t* samples, uint32_t sampleCnt);
static void puzzledropBackgroundDrawCallback(int16_t x, int16_t y, int16_t w,  int16_t h, int16_t up, int16_t upNum);
static void puzzledropEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void puzzledropEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static int16_t puzzledropAdvancedUSB(uint8_t* buffer, uint16_t length, uint8_t isGet);

#define STATE_DROPPING     1
#define STATE_CHECKING     2



swadgeMode_t puzzledropMode = {
    .modeName                   = puzzledropName,
    .wifiMode                   = ESP_NOW,
    .overrideUsb                = false,
    .usesAccelerometer          = true,
    .usesThermometer            = false,
    .overrideSelectBtn          = false,
    .fnEnterMode                = puzzledropEnterMode,
    .fnExitMode                 = puzzledropExitMode,
    .fnMainLoop                 = puzzledropMainLoop,
    .fnAudioCallback            = puzzledropAudioCallback,
    .fnBackgroundDrawCallback   = puzzledropBackgroundDrawCallback,
    .fnEspNowRecvCb             = puzzledropEspNowRecvCb,
    .fnEspNowSendCb             = puzzledropEspNowSendCb,
    .fnAdvancedUSB              = NULL,
};

puzzledropVars_t* puzzledrop;

static void puzzledropEnterMode(void)
{
    puzzledrop = calloc(1, sizeof(puzzledropVars_t));
    loadWsg("puzzledrop_background.wsg", &puzzledrop->tileBackground, true);
    loadWsg("puzzledrop_orange1.wsg", &puzzledrop->orangeTiles, false);
    loadFont("ibm_vga8.font", &puzzledrop->ibm_vga8, false);

    for (int i = 0; i < ARRAY_SIZE(puzzledrop->tiles); i++)
    {
        puzzledrop->tiles[i] = calloc(1, sizeof(puzzledropTile_t));
        puzzledrop->tiles[i]->active = false;
        puzzledrop->tiles[i]->x = -100;
        puzzledrop->tiles[i]->y = -100;
    }

    for (int i = 0; i < ARRAY_SIZE(puzzledrop->board); i++)
    {
        puzzledrop->board[i] = 0;
    }

    //ESP_LOGI("PUZ", "%d %d", TFT_HEIGHT, TFT_WIDTH );

    puzzledrop->currentMode = STATE_CHECKING;
}

static void puzzledropExitMode(void)
{
    
}

static void puzzledropMainLoop(int64_t elapsedUs)
{
    puzzledropVars_t* p = puzzledrop;

    //ESP_LOGI("PUZZ_GAME", "Update 0");
    p->frameTimer += elapsedUs;
    while (p->frameTimer >= PUZ_US_PER_FRAME)
    {
        p->frameTimer -= PUZ_US_PER_FRAME;
        //Update physics

        if (p->currentMode == STATE_DROPPING)
        {
            puzzledropUpdate(p, elapsedUs);
        }
        else if (p->currentMode == STATE_CHECKING)
        {
            puzzledropCheck(p);
        }
    }

    
    p->frameTimesIdx                = (p->frameTimesIdx + 1) % NUM_FRAME_TIMES;
    p->frameTimes[p->frameTimesIdx] = esp_timer_get_time();
}

static void puzzledropAudioCallback(uint16_t* samples, uint32_t sampleCnt)
{

}

static void puzzledropBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{

    //Draw Game
    fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, c145);
    
    puzzledropDraw(puzzledrop);
}

//static void rotatePixel(int16_t* x, int16_t* y, int16_t rotateDeg, int16_t width, int16_t height)
//
static void puzzledropEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{

}

static void puzzledropEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{

}

