/**
 * @file racer.c
 * @author Jonathan Moriarty (jonathan.taylor.moriarty@gmail.com)
 * @brief A mode 7 racing game styled after the SNES F-Zero
 * @date 2023-09-17
 *
 * TODO everything
 */

//==============================================================================
// Includes
//==============================================================================

#include "esp_random.h"
#include "racer.h"

//==============================================================================
// Defines
//==============================================================================

/// Physics math is done with fixed point numbers where the bottom four bits are the fractional part. It's like Q28.4
#define DECIMAL_BITS 4

#define SPEED_LIMIT (30 << DECIMAL_BITS)

//==============================================================================
// Enums
//==============================================================================

/**
 * @brief Enum of screens that may be shown in racer mode
 */
typedef enum
{
    RACER_MENU,
    RACER_GAME,
    RACER_SCORES,
} racerScreen_t;

/**
 * @brief Enum of control schemes for a racer game
 */
typedef enum
{
    RACER_BUTTON,
    RACER_TOUCH,
    RACER_TILT,
} racerControl_t;

/**
 * @brief Enum of CPU difficulties
 */
typedef enum
{
    RACER_EASY,
    RACER_MEDIUM,
    RACER_HARD,
} racerDifficulty_t;

//==============================================================================
// Structs
//==============================================================================

typedef struct
{
    menu_t* menu;                               ///< The menu structure
    menuLogbookRenderer_t* menuLogbookRenderer; ///< Renderer for the menu
    font_t ibm;                                 ///< The font used in the menu and game
    p2pInfo p2p;                                ///< Peer to peer connectivity info, currently unused
    racerScreen_t screen;                        ///< The screen being displayed

    racerControl_t control;       ///< The selected control scheme
    racerDifficulty_t difficulty; ///< The selected CPU difficulty

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for input control
    bool isPaused;          ///< true if the game is paused, false if it is running

    song_t bgm;  ///< Background music
} racer_t;

//==============================================================================
// Function Prototypes
//==============================================================================

static void racerMainLoop(int64_t elapsedUs);
static void racerEnterMode(void);
static void racerExitMode(void);
static void racerEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
static void racerEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);
static void racerMenuCb(const char* label, bool selected, uint32_t settingVal);
static void racerGameLoop(int64_t elapsedUs);

static void racerResetGame(bool isInit);
static void racerUpdatePhysics(int64_t elapsedUs);

static void racerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void racerDrawGame(void);

//==============================================================================
// Strings
//==============================================================================

/* Design Pattern!
 * These strings are all declared 'const' because they do not change, so that they are placed in ROM, not RAM.
 * Lengths are not explicitly given so the compiler can figure it out.
 */

static const char racerName[] = "Racer";

static const char racerCtrlButton[] = "Button Control";
static const char racerCtrlTouch[]  = "Touch Control";
static const char racerCtrlTilt[]   = "Tilt Control";

static const char racerDiffEasy[]   = "Easy";
static const char racerDiffMedium[] = "Medium";
static const char racerDiffHard[]   = "Impossible";

//static const char racerPaused[] = "Paused";

//==============================================================================
// Variables
//==============================================================================

/// The Swadge mode for Racer
swadgeMode_t racerMode = {
    .modeName                 = racerName,
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = true,
    .usesThermometer          = false,
    .fnEnterMode              = racerEnterMode,
    .fnExitMode               = racerExitMode,
    .fnMainLoop               = racerMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = racerBackgroundDrawCallback,
    .fnEspNowRecvCb           = racerEspNowRecvCb,
    .fnEspNowSendCb           = racerEspNowSendCb,
    .fnAdvancedUSB            = NULL,
};

/// All state information for the Racer mode. This whole struct is calloc()'d and free()'d so that Racer is only
/// using memory while it is being played
racer_t* racer = NULL;

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Enter Racer mode, allocate required memory, and initialize required variables
 *
 */
static void racerEnterMode(void)
{
    // Allocate and clear all memory for this mode. All the variables are contained in a single struct for convenience.
    // calloc() is used instead of malloc() because calloc() also initializes the allocated memory to zeros.
    racer = calloc(1, sizeof(racer_t));

    // Load a font
    loadFont("ibm_vga8.font", &racer->ibm, false);

    // Load graphics
    //loadWsg("pball.wsg", &racer->ballWsg, false);

    // Load SFX
    loadSong("stereo.sng", &racer->bgm, false);
    racer->bgm.shouldLoop = true;

    // Initialize the menu
    racer->menu                = initMenu(racerName, racerMenuCb);
    racer->menuLogbookRenderer = initMenuLogbookRenderer(&racer->ibm);

    // These are the possible control schemes
    const char* controlSchemes[] = {
        racerCtrlButton,
        racerCtrlTouch,
        racerCtrlTilt,
    };

    // Add each control scheme to the menu. Each control scheme has a submenu to select difficulty
    for (uint8_t i = 0; i < ARRAY_SIZE(controlSchemes); i++)
    {
        // Add a control scheme to the menu. This opens a submenu with difficulties
        racer->menu = startSubMenu(racer->menu, controlSchemes[i]);
        // Add difficulties to the submenu
        addSingleItemToMenu(racer->menu, racerDiffEasy);
        addSingleItemToMenu(racer->menu, racerDiffMedium);
        addSingleItemToMenu(racer->menu, racerDiffHard);
        // End the submenu
        racer->menu = endSubMenu(racer->menu);
    }

    // Set the mode to menu mode
    racer->screen = RACER_MENU;
}

/**
 * This function is called when the mode is exited. It deinitializes variables and frees all memory.
 */
static void racerExitMode(void)
{
    // Deinitialize the menu
    deinitMenu(racer->menu);
    deinitMenuLogbookRenderer(racer->menuLogbookRenderer);
    // Free the font
    freeFont(&racer->ibm);
    // Free graphics
    // Free the songs
    freeSong(&racer->bgm);
    // Free everything else
    free(racer);
}

/**
 * @brief This callback function is called when an item is selected from the menu
 *
 * @param label The item that was selected from the menu
 * @param selected True if the item was selected with the A button, false if this is a multi-item which scrolled to
 * @param settingVal The value of the setting, if the menu item is a settings item
 */
static void racerMenuCb(const char* label, bool selected, uint32_t settingVal)
{
    // Only care about selected items, not scrolled-to items.
    // The same callback is called from the menu and submenu with no indication of which menu it was called from
    // Note that the label arg will be one of the strings used in startSubMenu() or addSingleItemToMenu()
    if (selected)
    {
        // Save what control scheme is selected (first-level menu)
        if (label == racerCtrlButton)
        {
            racer->control = RACER_BUTTON;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == racerCtrlTouch)
        {
            racer->control = RACER_TOUCH;
        }
        // Save what control scheme is selected (first-level menu)
        else if (label == racerCtrlTilt)
        {
            racer->control = RACER_TILT;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == racerDiffEasy)
        {
            racer->difficulty = RACER_EASY;
            racerResetGame(true);
            racer->screen = RACER_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == racerDiffMedium)
        {
            racer->difficulty = RACER_MEDIUM;
            racerResetGame(true);
            racer->screen = RACER_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == racerDiffHard)
        {
            racer->difficulty = RACER_HARD;
            racerResetGame(true);
            racer->screen = RACER_GAME;
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It will either draw the menu or play the game, depending
 * on which screen is currently being displayed
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void racerMainLoop(int64_t elapsedUs)
{
    // Pick what runs and draws depending on the screen being displayed
    switch (racer->screen)
    {
        case RACER_MENU:
        {
            // Process button events
            buttonEvt_t evt = {0};
            while (checkButtonQueueWrapper(&evt))
            {
                // Pass button events to the menu
                racer->menu = menuButton(racer->menu, evt);
            }

            // Draw the menu
            drawMenuLogbook(racer->menu, racer->menuLogbookRenderer, elapsedUs);
            break;
        }
        case RACER_GAME:
        {
            // Run the main game loop. This will also process button events
            racerGameLoop(elapsedUs);
            break;
        }
    }
}

/**
 * @brief This function is called periodically and frequently. It runs the actual game, including processing inputs,
 * physics updates and drawing to the display.
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void racerGameLoop(int64_t elapsedUs)
{
    // Always process button events, regardless of control scheme, so the main menu button can be captured
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        // Save the button state
        racer->btnState = evt.state;

        // Check if the pause button was pressed
        if (evt.down && (PB_START == evt.button))
        {
            // Toggle pause
            racer->isPaused = !racer->isPaused;
        }
    }

    // If the game is paused
    if (racer->isPaused)
    {
        // Just draw and return
        racerDrawGame();
        return;
    }

    // While the restart timer is active
    while (racer->restartTimerUs > 0)
    {
        // Decrement the timer and draw the field, but don't run game logic
        racer->restartTimerUs -= elapsedUs;
        racerDrawGame();
        return;
    }

    // Do update each loop
    racerUpdatePhysics(elapsedUs);

    // Draw the field
    racerDrawGame();
}

/**
 * @brief Update the racer physics including ball position and collisions
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void racerUpdatePhysics(int64_t elapsedUs)
{
    
}

/**
 * @brief Reset the racer game variables
 *
 * @param isInit True if this is the first time this is being called, false if it is reset after a player scores
 * @param whoWon The player who scored a point, either 0 or 1
 */
static void racerResetGame(bool isInit)
{
    // Set different variables based on initialization
    if (isInit)
    {
        
    }
    else
    {
        
    }
}

/**
 * This function is called when the display driver wishes to update a
 * section of the display.
 *
 * @param disp The display to draw to
 * @param x the x coordinate that should be updated
 * @param y the x coordinate that should be updated
 * @param w the width of the rectangle to be updated
 * @param h the height of the rectangle to be updated
 * @param up update number
 * @param numUp update number denominator
 */
static void racerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    // Use TURBO drawing mode to draw individual pixels fast
    SETUP_FOR_TURBO();

    // Draw a grid
    for (int16_t yp = y; yp < y + h; yp++)
    {
        for (int16_t xp = x; xp < x + w; xp++)
        {
            if ((0 == xp % 40) || (0 == yp % 40))
            {
                TURBO_SET_PIXEL(xp, yp, c110);
            }
            else
            {
                TURBO_SET_PIXEL(xp, yp, c001);
            }
        }
    }
}

/**
 * @brief Draw the Racer field to the TFT
 */
static void racerDrawGame(void)
{
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);

    // No need to clear the display before drawing because it's redrawn by racerBackgroundDrawCallback() each time

    // This will draw the game with sprites, not geometric shapes
}

/**
 * This function is called whenever an ESP-NOW packet is received.
 *
 * @param esp_now_info Information about the transmission, including The MAC addresses
 * @param data A pointer to the data received
 * @param len The length of the data received
 * @param rssi The RSSI for this packet, from 1 (weak) to ~90 (touching)
 */
static void racerEspNowRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&racer->p2p, esp_now_info->src_addr, data, len, rssi);
}

/**
 * This function is called whenever an ESP-NOW packet is sent.
 * It is just a status callback whether or not the packet was actually sent.
 * This will be called after calling espNowSend()
 *
 * @param mac_addr The MAC address which the data was sent to
 * @param status The status of the transmission
 */
static void racerEspNowSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&racer->p2p, mac_addr, status);
}
