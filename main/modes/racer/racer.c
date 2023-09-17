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

#define BALL_RADIUS   (5 << DECIMAL_BITS)
#define PADDLE_WIDTH  (8 << DECIMAL_BITS)
#define PADDLE_HEIGHT (40 << DECIMAL_BITS)
#define FIELD_HEIGHT  (TFT_HEIGHT << DECIMAL_BITS)
#define FIELD_WIDTH   (TFT_WIDTH << DECIMAL_BITS)

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
    uint8_t score[2];            ///< The score for the game

    rectangle_t paddleL; ///< The left paddle
    rectangle_t paddleR; ///< The right paddle
    circle_t ball;       ///< The ball
    vec_t ballVel;       ///< The ball's velocity

    int32_t restartTimerUs; ///< A timer that counts down before the game begins
    uint16_t btnState;      ///< The button state used for paddle control
    bool paddleRMovingUp;   ///< The CPU's paddle direction on easy mode
    bool isPaused;          ///< true if the game is paused, false if it is running

    wsg_t paddleWsg; ///< A graphic for the paddle
    wsg_t ballWsg;   ///< A graphic for the ball

    song_t bgm;  ///< Background music
    song_t hit1; ///< Sound effect for one paddle's hit
    song_t hit2; ///< Sound effect for the other paddle's hit

    led_t ledL;           ///< The left LED color
    led_t ledR;           ///< The right LED color
    int32_t ledFadeTimer; ///< The timer to fade LEDs
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

static void racerResetGame(bool isInit, uint8_t whoWon);
static void racerFadeLeds(int64_t elapsedUs);
static void racerControlPlayerPaddle(void);
static void racerControlCpuPaddle(void);
static void racerUpdatePhysics(int64_t elapsedUs);
static void racerIncreaseBallVelocity(int16_t magnitude);

static void racerBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
static void racerDrawField(void);

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

static const char racerPaused[] = "Paused";

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

    // Below, various assets are loaded from the SPIFFS file system to RAM. How did they get there?
    // The source assets are found in the /assets/racer/ directory. Each asset is processed and packed into the SPIFFS
    // file system at compile time The following transformations are made:
    // * pball.png   -> pball.wsg
    // * ppaddle.png -> ppaddle.wsg
    // * block1.mid  -> block1.sng
    // * block2.mid  -> block2.sng
    // * gmcc.mid    -> gmcc.sng
    //
    // In addition, a common font is found in /assets/fonts/ and is transformed like so:
    // * ibm_vga8.font.png -> ibm_vga8.font
    //
    // If you'd like to learn more about how assets are processed and packed, see
    // /tools/spiffs_file_preprocessor/README.md
    //
    // If you'd like to learn more about how assets are loaded, see
    // /components/hdw-spiffs/include/hdw-spiffs.h

    // Load a font
    loadFont("ibm_vga8.font", &racer->ibm, false);

    // Load graphics
    loadWsg("pball.wsg", &racer->ballWsg, false);
    loadWsg("ppaddle.wsg", &racer->paddleWsg, false);

    // Load SFX
    loadSong("block1.sng", &racer->hit1, false);
    loadSong("block2.sng", &racer->hit2, false);
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
    freeWsg(&racer->ballWsg);
    freeWsg(&racer->paddleWsg);
    // Free the songs
    freeSong(&racer->bgm);
    freeSong(&racer->hit1);
    freeSong(&racer->hit2);
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
            racerResetGame(true, 0);
            racer->screen = RACER_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == racerDiffMedium)
        {
            racer->difficulty = RACER_MEDIUM;
            racerResetGame(true, 0);
            racer->screen = RACER_GAME;
        }
        // Save what difficulty is selected and start the game (second-level menu)
        else if (label == racerDiffHard)
        {
            racer->difficulty = RACER_HARD;
            racerResetGame(true, 0);
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
        racerDrawField();
        return;
    }

    // While the restart timer is active
    while (racer->restartTimerUs > 0)
    {
        // Decrement the timer and draw the field, but don't run game logic
        racer->restartTimerUs -= elapsedUs;
        racerDrawField();
        return;
    }

    // Do update each loop
    racerFadeLeds(elapsedUs);
    racerControlPlayerPaddle();
    racerControlCpuPaddle();
    racerUpdatePhysics(elapsedUs);

    // Draw the field
    racerDrawField();
}

/**
 * @brief Fade the LEDs at a consistent rate over time
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void racerFadeLeds(int64_t elapsedUs)
{
    // This timer fades out LEDs. The fade is checked every 10ms
    // The pattern of incrementing a variable by elapsedUs, then decrementing it when it accumulates
    racer->ledFadeTimer += elapsedUs;
    while (racer->ledFadeTimer >= 10000)
    {
        racer->ledFadeTimer -= 10000;

        // Fade left LED channels independently
        if (racer->ledL.r)
        {
            racer->ledL.r--;
        }
        if (racer->ledL.g)
        {
            racer->ledL.g--;
        }
        if (racer->ledL.b)
        {
            racer->ledL.b--;
        }

        // Fade right LEDs channels independently
        if (racer->ledR.r)
        {
            racer->ledR.r--;
        }
        if (racer->ledR.g)
        {
            racer->ledR.g--;
        }
        if (racer->ledR.b)
        {
            racer->ledR.b--;
        }
    }
}

/**
 * @brief Move the player's paddle according to the chosen control scheme
 */
static void racerControlPlayerPaddle(void)
{
    // Move the paddle depending on the chosen control scheme
    switch (racer->control)
    {
        default:
        case RACER_BUTTON:
        {
            // Move the player paddle if a button is currently down
            if (racer->btnState & PB_UP)
            {
                racer->paddleL.y = MAX(racer->paddleL.y - (5 << DECIMAL_BITS), 0);
            }
            else if (racer->btnState & PB_DOWN)
            {
                racer->paddleL.y = MIN(racer->paddleL.y + (5 << DECIMAL_BITS), FIELD_HEIGHT - racer->paddleL.height);
            }
            break;
        }
        case RACER_TOUCH:
        {
            // Check if the touch area is touched
            int32_t phi, r, intensity;
            if (getTouchJoystick(&phi, &r, &intensity))
            {
                // If there is a touch, move the paddle to that location of the touch
                int paddley = phi - 320;
                if (paddley < 0)
                {
                    paddley = 0;
                }
                if (paddley >= 640)
                {
                    paddley = 639;
                }
                racer->paddleL.y = (paddley * (FIELD_HEIGHT - racer->paddleL.height)) / 640;
            }
            break;
        }
        case RACER_TILT:
        {
            // Declare variables to receive acceleration
            int16_t a_x, a_y, a_z;
            // Get the current acceleration
            if (ESP_OK == accelGetAccelVec(&a_x, &a_y, &a_z))
            {
                // Move the paddle to the current tilt location
                racer->paddleL.y = CLAMP(((a_x + 100) * (FIELD_HEIGHT - racer->paddleL.height)) / 350, 0,
                                        (FIELD_HEIGHT - racer->paddleL.height));
            }
            break;
        }
    }
}

/**
 * @brief Move the CPU's paddle according to the chosen difficulty
 */
static void racerControlCpuPaddle(void)
{
    // Move the computer paddle
    switch (racer->difficulty)
    {
        default:
        case RACER_EASY:
        {
            // Blindly move up and down
            if (racer->paddleRMovingUp)
            {
                racer->paddleR.y = MAX(racer->paddleR.y - (4 << DECIMAL_BITS), 0);
                // If the top boundary was hit
                if (0 == racer->paddleR.y)
                {
                    // Start moving down
                    racer->paddleRMovingUp = false;
                }
            }
            else
            {
                racer->paddleR.y = MIN(racer->paddleR.y + (4 << DECIMAL_BITS), FIELD_HEIGHT - racer->paddleR.height);
                // If the bottom boundary was hit
                if ((FIELD_HEIGHT - racer->paddleR.height) == racer->paddleR.y)
                {
                    // Start moving up
                    racer->paddleRMovingUp = true;
                }
            }
            break;
        }
        case RACER_MEDIUM:
        {
            // Move towards the ball, slowly
            if (racer->paddleR.y + (racer->paddleR.height / 2) < racer->ball.y)
            {
                racer->paddleR.y = MIN(racer->paddleR.y + (2 << DECIMAL_BITS), FIELD_HEIGHT - racer->paddleR.height);
            }
            else
            {
                racer->paddleR.y = MAX(racer->paddleR.y - (2 << DECIMAL_BITS), 0);
            }
            break;
        }
        case RACER_HARD:
        {
            // Never miss
            racer->paddleR.y = CLAMP(racer->ball.y - racer->paddleR.height / 2, 0, FIELD_HEIGHT - racer->paddleR.height);
            break;
        }
    }
}

/**
 * @brief Update the racer physics including ball position and collisions
 *
 * @param elapsedUs The time that has elapsed since the last call to this function, in microseconds
 */
static void racerUpdatePhysics(int64_t elapsedUs)
{
    // Update the ball's position
    racer->ball.x += (racer->ballVel.x * elapsedUs) / 100000;
    racer->ball.y += (racer->ballVel.y * elapsedUs) / 100000;

    // Check for goals
    if (racer->ball.x < 0 || racer->ball.x > FIELD_WIDTH)
    {
        // Reset the game. This keeps score.
        racerResetGame(false, racer->ball.x < 0 ? 1 : 0);
    }
    else
    {
        // Checks for top and bottom wall collisions
        if (((racer->ball.y - racer->ball.radius) < 0 && racer->ballVel.y < 0)
            || ((racer->ball.y + racer->ball.radius) > FIELD_HEIGHT && racer->ballVel.y > 0))
        {
            // Reverse direction
            racer->ballVel.y = -racer->ballVel.y;
        }

        // If a goal was not scored, check for left paddle collision
        if ((racer->ballVel.x < 0) && circleRectIntersection(racer->ball, racer->paddleL))
        {
            // Reverse direction
            racer->ballVel.x = -racer->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = racer->ball.y - (racer->paddleL.y + racer->paddleL.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            racer->ballVel = rotateVec2d(racer->ballVel, (45 * diff) / (racer->paddleL.height / 2));

            // Increase velocity
            racerIncreaseBallVelocity(1 << DECIMAL_BITS);

            // Play SFX
            bzrPlaySfx(&racer->hit1, BZR_LEFT);

            // Set an LED
            racer->ledL.r = 0xFF;
            racer->ledL.g = 0x80;
            racer->ledL.b = 0x40;
        }
        // Check for right paddle collision
        else if ((racer->ballVel.x > 0) && circleRectIntersection(racer->ball, racer->paddleR))
        {
            // Reverse direction
            racer->ballVel.x = -racer->ballVel.x;

            // Apply extra rotation depending on part of the paddle hit
            int16_t diff = racer->ball.y - (racer->paddleR.y + racer->paddleR.height / 2);
            // rotate 45deg at edge of paddle, 0deg in middle, linear in between
            racer->ballVel = rotateVec2d(racer->ballVel, -(45 * diff) / (racer->paddleR.height / 2));

            // Increase velocity
            racerIncreaseBallVelocity(1 << DECIMAL_BITS);

            // Play SFX
            bzrPlaySfx(&racer->hit2, BZR_RIGHT);

            // Set an LED
            racer->ledR.r = 0x40;
            racer->ledR.g = 0x80;
            racer->ledR.b = 0xFF;
        }
    }
}

/**
 * @brief Reset the racer game variables
 *
 * @param isInit True if this is the first time this is being called, false if it is reset after a player scores
 * @param whoWon The player who scored a point, either 0 or 1
 */
static void racerResetGame(bool isInit, uint8_t whoWon)
{
    // Set different variables based on initialization
    if (isInit)
    {
        // Set up the left paddle
        racer->paddleL.x      = 0;
        racer->paddleL.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        racer->paddleL.width  = PADDLE_WIDTH;
        racer->paddleL.height = PADDLE_HEIGHT;

        // Set up the right paddle
        racer->paddleR.x      = FIELD_WIDTH - PADDLE_WIDTH;
        racer->paddleR.y      = (FIELD_HEIGHT - PADDLE_HEIGHT) / 2;
        racer->paddleR.width  = PADDLE_WIDTH;
        racer->paddleR.height = PADDLE_HEIGHT;

        // Start playing music
        bzrPlayBgm(&racer->bgm, BZR_STEREO);
    }
    else
    {
        // Tally the score
        racer->score[whoWon]++;
    }

    // Set the restart timer
    racer->restartTimerUs = 2000000;

    // Set the ball variables
    racer->ball.x      = (FIELD_WIDTH) / 2;
    racer->ball.y      = (FIELD_HEIGHT) / 2;
    racer->ball.radius = BALL_RADIUS;

    // Give the ball initial velocity to the top right
    racer->ballVel.x = (4 << DECIMAL_BITS);
    racer->ballVel.y = -(4 << DECIMAL_BITS);

    // Rotate up to 90 degrees, for some randomness
    racer->ballVel = rotateVec2d(racer->ballVel, esp_random() % 90);

    // Determine the direction of the serve based on who scored last
    if (whoWon)
    {
        racer->ballVel.x = -racer->ballVel.x;
    }
}

/**
 * @brief Increase the ball's velocity by a fixed amount
 *
 * @param magnitude The magnitude velocity to add to the ball
 */
static void racerIncreaseBallVelocity(int16_t magnitude)
{
    if (sqMagVec2d(racer->ballVel) < (SPEED_LIMIT * SPEED_LIMIT))
    {
        // Create a vector in the same direction as racer->ballVel with the given magnitude
        int32_t denom  = ABS(racer->ballVel.x) + ABS(racer->ballVel.y);
        vec_t velBoost = {
            .x = (magnitude * racer->ballVel.x) / denom,
            .y = (magnitude * racer->ballVel.y) / denom,
        };

        // Add the vectors together
        racer->ballVel = addVec2d(racer->ballVel, velBoost);
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
static void racerDrawField(void)
{
    // Create an array for all LEDs
    led_t leds[CONFIG_NUM_LEDS];
    // Copy the LED colors for left and right to the whole array
    for (uint8_t i = 0; i < CONFIG_NUM_LEDS / 2; i++)
    {
        leds[i]                         = racer->ledL;
        leds[i + (CONFIG_NUM_LEDS / 2)] = racer->ledR;
    }
    // Set the LED output
    setLeds(leds, CONFIG_NUM_LEDS);

    // No need to clear the display before drawing because it's redrawn by racerBackgroundDrawCallback() each time

#ifdef DRAW_SHAPES
    // This will draw the game with geometric shapes, not sprites

    // Bitshift the ball's location and radius from math coordinates to screen coordinates, then draw it
    drawCircleFilled((racer->ball.x) >> DECIMAL_BITS, (racer->ball.y) >> DECIMAL_BITS,
                     (racer->ball.radius) >> DECIMAL_BITS, c555);

    // Bitshift the left paddle's location and radius from math coordinates to screen coordinates, then draw it
    drawRect((racer->paddleL.x >> DECIMAL_BITS), (racer->paddleL.y >> DECIMAL_BITS),
             ((racer->paddleL.x + racer->paddleL.width) >> DECIMAL_BITS),
             ((racer->paddleL.y + racer->paddleL.height) >> DECIMAL_BITS), c333);
    // Bitshift the right paddle's location and radius from math coordinates to screen coordinates, then draw it

    drawRect((racer->paddleR.x >> DECIMAL_BITS), (racer->paddleR.y >> DECIMAL_BITS),
             ((racer->paddleR.x + racer->paddleR.width) >> DECIMAL_BITS),
             ((racer->paddleR.y + racer->paddleR.height) >> DECIMAL_BITS), c333);
#else
    // This will draw the game with sprites, not geometric shapes

    // Draw the ball
    drawWsgSimple(&racer->ballWsg, (racer->ball.x - racer->ball.radius) >> DECIMAL_BITS,
                  (racer->ball.y - racer->ball.radius) >> DECIMAL_BITS);
    // Draw one paddle
    drawWsg(&racer->paddleWsg, racer->paddleL.x >> DECIMAL_BITS, racer->paddleL.y >> DECIMAL_BITS, false, false, 0);
    // Draw the other paddle, flipped
    drawWsg(&racer->paddleWsg, racer->paddleR.x >> DECIMAL_BITS, racer->paddleR.y >> DECIMAL_BITS, true, false, 0);

#endif

    // Set up variables to draw text
    char scoreStr[16] = {0};
    int16_t tWidth;

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, racer->score[0]);
    // Measure the width of the score string
    tWidth = textWidth(&racer->ibm, scoreStr);
    // Draw the score string to the display, centered at (TFT_WIDTH / 4)
    drawText(&racer->ibm, c555, scoreStr, (TFT_WIDTH / 4) - (tWidth / 2), 0);

    // Render a number to a string
    snprintf(scoreStr, sizeof(scoreStr) - 1, "%" PRIu8, racer->score[1]);
    // Measure the width of the score string
    tWidth = textWidth(&racer->ibm, scoreStr);
    // Draw the score string to the display, centered at ((3 * TFT_WIDTH) / 4)
    drawText(&racer->ibm, c555, scoreStr, ((3 * TFT_WIDTH) / 4) - (tWidth / 2), 0);

    // If the restart timer is active, draw it
    if (racer->isPaused)
    {
        // Measure the width of the time string
        tWidth = textWidth(&racer->ibm, racerPaused);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&racer->ibm, c555, racerPaused, ((TFT_WIDTH - tWidth) / 2), 0);
    }
    else if (racer->restartTimerUs > 0)
    {
        // Render the time to a string
        snprintf(scoreStr, sizeof(scoreStr) - 1, "%01" PRId32 ".%03" PRId32, racer->restartTimerUs / 1000000,
                 (racer->restartTimerUs / 1000) % 1000);
        // Measure the width of the time string
        tWidth = textWidth(&racer->ibm, scoreStr);
        // Draw the time string to the display, centered at (TFT_WIDTH / 2)
        drawText(&racer->ibm, c555, scoreStr, ((TFT_WIDTH - tWidth) / 2), 0);
    }
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
