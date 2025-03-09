/**
Compile with the code below

windres resource.rc -O coff -o resource.res
gcc main.cpp -o main.exe -Wall -Wno-missing-braces -I include/ -L lib/ -lraylib -lopengl32 -lgdi32 -lwinmm -mwindows -lstdc++ resource.res
*/

/**
 * cdRayer, a simple-to-edit, lightweight CD Player Overlay
 * So far only tested on Windows 10.
 *
 * FEATURES
 * - Spotify sync!
 * - Transparent background
 * - Draggable window
 * - Always on top
 * - Thoroughly commented code! This app *wants* you to poke it with a stick
 *
 * CONTROLS
 * - SPACEBAR - Toggle on/off
 * - ESC - Close app
 * - S - Swap between the current and last album cover. Used when the program has a hiccup and forgets to switch automatically.
 *
 * HOW TO ADD YOUR OWN COVER
 * - Step 0: Download cdRayer v.01 or v.011
 * - Step 1: Have an image editing software installed that has clipping and radial blur (alternatively, look up "radial blur online" on your browser)
 * - Step 2: Open the app folder, "assets", from there open "cdCover.png" or "cdCoverFast.png"
 * - Step 3: Add a layer on top. Place your album cover so it is scaled with the window. Then clip
 * - Step 4: Export as "cdCover.png" inside "assets" in the app folder
 * - Step 5: Apply radial blur to your photo. Repeat steps 3 and 4, but export as "cdCoverFast.png"
 * - You should be done!
 * 
 * HOW TO SYNC TO SPOTIFY
 * - Step 0: Download cdRayer v.011 S
 * - Step 1: Open Spotify for Developers, Log In, Dashboard, Create App, Web API, write "http://localhost/" for Redirect URI.
 * - Step 2: Open the folder "pythonSpotifyAPI", "getCurrentSongInfo.py", and write your clientID and clientSecret as they appear on your Spotify for Developers project.
 * - Step 3: Run the program, it'll probably tell you in the terminal to paste some URL you get redirected to. Do that.
 * - Step 4: In the terminal, paste the command found at the top of the file. That will compile this getCurrentSongInfo script into an exe.
 * - Step 5: Open getCurrentSongInfo.exe and main.exe. cdRayer should now sync with your Spotify.
 * - DISCLAIMER - "getCurrentSongInfo.exe" does NOT appear on the taskbar, and it does NOT sync its opening/closing with "main.exe". It must be closed through the task manager.
 *
 * Suggestions, feedback, or just a conversation are more than welcome on my email ^^ collie.dawg.8075@gmail.com
 *
 * Almost all declared variables are not set in stone. Play around!!
 * :3
 */

#include <cstdlib>
#include <cmath>
#include "include/raylib.h"
#include <filesystem>
#include <cstring>

namespace fs = std::filesystem;
int songChangedFlag = 1, cdCoverPingPong = 10000, windowWait = 0;
bool alreadyChanged = false;

typedef enum
{
    CLOSING,
    OPENING
} WindowState;

WindowState windowState = OPENING;
float windowSpeed = 0.0f;
float windowAcceleration = 0.02f;

typedef enum
{
    ACCELERATING,
    DECELERATING
} AccelState;

typedef enum
{
    SPIN_STOPPED,
    SPIN_STARTING,
    SPIN_RUNNING,
    SPIN_STOPPING
} CdSpinState;

float cdSize = 300.0f;                         // cdSize also influences the window size. Currently only displays correctly for 200.0f -> 400.0f. Things are offset otherwise
float cdNippleSize = cdSize * 0.3f;            // Currently only displays correctly for 0.3f TwT I tried finding a formula that correctly adjusts for cdSize and nippleSize but couldn't
float cdPlayerScale = 1.01f;                   // Better called "cdPlayerSquishScale". The higher it is, the more squished the cd player will be. Does not affect cdPlayerTop. "Default" would be 1.005f. Best values are between 1.0f and 1.5f, but play around with it
float cdPlayerHeight = cdSize / cdPlayerScale; // For now, fixed value that works with the current cd player skin
float cdPlayerWidth = cdSize / 1.005f * 1.42f; // ^

float windowWidth = cdSize * 1.5f, windowHeight = cdSize * 1.2f; // The bigger the cd, the bigger the window

float cdRotationSpeed = 0.0f;                                                                 // self explanatory
float cdDesiredRotationSpeed = 45.5f;                                                         // self explanatory
AccelState accelState = ACCELERATING;                                                         // not to be confused with STARTING. This is the cd's state when in motion
float cdAcceleration = cdDesiredRotationSpeed;                                                // self explanatory
float cdAccelerationSpeed = 0.1f;                                                             // self explanatory
float cdRotation = 0.0f;                                                                      // self explanatory
float cdOverlayRotation = 0.0f, cdOverlayRotationGoal = rand() % 720, cdOverlaySpeed = 10.0f; // cdOverlayRotationGoal tells cdOverlayRotation what angle to spin to. Best value that I found for cdOverlaySpeed is either a small one such as 3.0f or a high one such as 20.0f

CdSpinState cdSpinState = SPIN_STOPPED; // self explanatory
float transitionRotateSpeed = 0.001f;   // Used in the stop -> start or start -> stop transition to smooth out the speed changes

Vector2 lastMousePosition = {0}; // Used for window dragging part 1
Vector2 dragOffset = {0};
bool dragging = false;

float refreshRateMultiplier = 1;

int main()
{

    SetConfigFlags(FLAG_WINDOW_TRANSPARENT);
    SetConfigFlags(FLAG_WINDOW_TOPMOST);
    InitWindow(windowWidth, windowHeight, "cdRayer");
    SetWindowPosition(1920 - windowWidth, 1080 - windowHeight - 100);                    // Places the window on the bottom left at startup
    SetTargetFPS(GetMonitorRefreshRate(GetCurrentMonitor()));                            // Change this to 60 if needed. Keep in mind, things *will* appear slower, since the app does less updates per second
    refreshRateMultiplier = -0.01667 * GetMonitorRefreshRate(GetCurrentMonitor()) + 3.4; // Scuffed function that (should) returns 2.4 for 60 and 1 for 144, and everything inbetween. Fixes (or SHOULD fix) the slower speed on slower refresh rates

    SetWindowState(FLAG_WINDOW_UNDECORATED);

    // Preparing the textures

    // windowBg. Maybe it fixes the flashing?
    char windowBgPath[] = "assets/windowBg.png";
    Image windowBgImage = LoadImage(windowBgPath);
    Texture2D windowBgTexture = LoadTextureFromImage(windowBgImage);
    UnloadImage(windowBgImage);

    // cdOverlay
    char cdOverlayPath[] = "assets/cdOverlay1.png";
    Image cdOverlay = LoadImage(cdOverlayPath);
    Texture2D cdOverlayTexture = LoadTextureFromImage(cdOverlay);
    UnloadImage(cdOverlay);

    BlendMode cdOverlayBlendMode = BLEND_ADDITIVE;

    // cdClear
    char cdClearPath[100] = "assets/cdCover.png";
    Image cdClearImage;
    // Texture2D cdClearTexture = LoadTextureFromImage(cdClearImage);
    Texture2D cdClearTexture;
    // UnloadImage(cdClearImage);

    // cdNipple
    char cdNipplePath[] = "assets/nipple.png";
    Image cdNipple = LoadImage(cdNipplePath);
    Texture2D cdNippleTexture = LoadTextureFromImage(cdNipple);
    UnloadImage(cdNipple);

    // cdPlayerBottomShadow
    char cdPlayerBottomShadowPath[] = "assets/cdPlayerBottomShadow.png";
    Image cdPlayerBottomShadowImage = LoadImage(cdPlayerBottomShadowPath);
    Texture2D cdPlayerBottomShadowTexture = LoadTextureFromImage(cdPlayerBottomShadowImage);
    UnloadImage(cdPlayerBottomShadowImage);

    // cdPlayerBottom
    char cdPlayerBottomPath[] = "assets/cdPlayerBottom.png";
    Image cdPlayerBottomImage = LoadImage(cdPlayerBottomPath);
    Texture2D cdPlayerBottomTexture = LoadTextureFromImage(cdPlayerBottomImage);
    UnloadImage(cdPlayerBottomImage);

    // cdPlayerBottomNotScaled
    char cdPlayerBottomNotScaledPath[] = "assets/cdPlayerBottomNotScaled.png";
    Image cdPlayerBottomNotScaledImage = LoadImage(cdPlayerBottomNotScaledPath);
    Texture2D cdPlayerBottomNotScaledTexture = LoadTextureFromImage(cdPlayerBottomNotScaledImage);
    UnloadImage(cdPlayerBottomNotScaledImage);

    // cdPlayerTop
    char cdPlayerTopPath[] = "assets/cdPlayerTop.png";
    Image cdPlayerTopImage = LoadImage(cdPlayerTopPath);
    Texture2D cdPlayerTopTexture = LoadTextureFromImage(cdPlayerTopImage);
    UnloadImage(cdPlayerTopImage);

    RenderTexture2D target = LoadRenderTexture(windowWidth, windowHeight);

    // Starting the window

    while (WindowShouldClose() == false)
    {
        BeginTextureMode(target);
        ClearBackground(BLANK);
        EndTextureMode();

        // Event Handling

        if (fs::exists("pythonSpotifyAPI\\songChangedFlag1.txt"))
        {
            songChangedFlag = 1;
            windowState = CLOSING;
            std::remove("pythonSpotifyAPI\\songChangedFlag1.txt");
            alreadyChanged = false;
        }
        if (fs::exists("pythonSpotifyAPI\\songChangedFlag2.txt"))
        {
            songChangedFlag = 2;
            windowState = CLOSING;
            std::remove("pythonSpotifyAPI\\songChangedFlag2.txt");
            alreadyChanged = false;
        }

        if (IsKeyPressed(KEY_S))
        {
            songChangedFlag = 1;
            alreadyChanged = false;
        }

        // printf("%d\n", songChangedFlag);
        if (songChangedFlag == 2)
            switch (windowState)
            {
            case CLOSING:
                windowSpeed += windowAcceleration;
                if (GetWindowPosition().x < 1920 - windowWidth)
                    SetWindowPosition(GetWindowPosition().x + windowSpeed * 2, GetWindowPosition().y);
                else
                    SetWindowPosition(GetWindowPosition().x + windowSpeed / 5, GetWindowPosition().y);
                printf("%.6f\n", GetWindowPosition().x);
                if (GetWindowPosition().x > 1900)
                {
                    if (windowWait > 30)
                    {
                        windowState = OPENING;
                        windowSpeed = 0.0f;
                        windowWait = 0;
                    }
                    windowWait++;
                }
                alreadyChanged = false;
                break;
            case OPENING:

                cdSpinState = SPIN_STARTING;
                transitionRotateSpeed = 0.0f;
                cdAccelerationSpeed = 0.1f;
                cdRotation = 0.0f;
                cdRotationSpeed = 0.0f;
                if (cdCoverPingPong == 9999)
                {
                    if (alreadyChanged == false)
                    {
                        strcpy(cdClearPath, "assets/cdCover9999.png");
                        cdClearPath[strlen(cdClearPath)] = NULL;
                        cdClearImage = LoadImage(cdClearPath);
                        UnloadTexture(cdClearTexture);
                        cdClearTexture = LoadTextureFromImage(cdClearImage);
                        UnloadImage(cdClearImage);
                        alreadyChanged = true;
                    }
                    cdCoverPingPong++;
                }
                else
                {
                    if (alreadyChanged == false)
                    {
                        strcpy(cdClearPath, "assets/cdCover10000.png");
                        cdClearPath[strlen(cdClearPath)] = NULL;
                        cdClearImage = LoadImage(cdClearPath);
                        UnloadTexture(cdClearTexture);
                        cdClearTexture = LoadTextureFromImage(cdClearImage);
                        UnloadImage(cdClearImage);
                        alreadyChanged = true;
                    }
                    cdCoverPingPong--;
                }

                windowSpeed += windowAcceleration;
                SetWindowPosition(GetWindowPosition().x - windowSpeed, GetWindowPosition().y);
                printf("%.6f\n", GetWindowPosition().x);
                if (GetWindowPosition().x - 100 < 1920 - windowWidth)
                {
                    songChangedFlag = 0;
                }
                break;
            }
        else if (songChangedFlag == 1)
        switch (windowState)
        {
        case CLOSING:
            windowSpeed += windowAcceleration;
            if (GetWindowPosition().x < 1920 - windowWidth)
                SetWindowPosition(GetWindowPosition().x + windowSpeed * 2, GetWindowPosition().y);
            else
                SetWindowPosition(GetWindowPosition().x + windowSpeed / 5, GetWindowPosition().y);
            printf("%.6f\n", GetWindowPosition().x);
            if (GetWindowPosition().x > 1900)
            {
                if (windowWait > 100)
                {
                    windowState = OPENING;
                    windowSpeed = 0.0f;
                    windowWait = 0;
                }
                windowWait++;
            }
            alreadyChanged = false;
            break;
        case OPENING:

            cdSpinState = SPIN_STARTING;
            transitionRotateSpeed = 0.0f;
            cdAccelerationSpeed = 0.1f;
            cdRotation = 0.0f;
            cdRotationSpeed = 0.0f;
            if (cdCoverPingPong == 9999)
            {
                if (alreadyChanged == false)
                {
                    strcpy(cdClearPath, "assets/cdCover9999.png");
                    cdClearPath[strlen(cdClearPath)] = NULL;
                    cdClearImage = LoadImage(cdClearPath);
                    UnloadTexture(cdClearTexture);
                    cdClearTexture = LoadTextureFromImage(cdClearImage);
                    UnloadImage(cdClearImage);
                    alreadyChanged = true;
                }
                cdCoverPingPong++;
            }
            else
            {
                if (alreadyChanged == false)
                {
                    strcpy(cdClearPath, "assets/cdCover10000.png");
                    cdClearPath[strlen(cdClearPath)] = NULL;
                    cdClearImage = LoadImage(cdClearPath);
                    UnloadTexture(cdClearTexture);
                    cdClearTexture = LoadTextureFromImage(cdClearImage);
                    UnloadImage(cdClearImage);
                    alreadyChanged = true;
                }
                cdCoverPingPong--;
            }

            windowSpeed += windowAcceleration;
            SetWindowPosition(GetWindowPosition().x - windowSpeed, GetWindowPosition().y);
            printf("%.6f\n", GetWindowPosition().x);
            if (GetWindowPosition().x - 100 < 1920 - windowWidth)
            {
                songChangedFlag = 0;
            }
            break;
        }

        switch (cdSpinState)
        {
        case SPIN_STOPPED:

            if (IsKeyPressed(KEY_SPACE))
            {
                transitionRotateSpeed = 0.001f;
                cdSpinState = SPIN_STARTING;
            }
            break;

        case SPIN_STARTING:

            // accelerating cdRotationSpeed
            transitionRotateSpeed += 0.0001f;
            cdRotationSpeed += transitionRotateSpeed;
            cdRotation += cdRotationSpeed;

            cdOverlayRotation -= cdOverlaySpeed * 0.001f * (cdOverlayRotation - cdOverlayRotationGoal) * cdRotationSpeed * 0.03f;

            if (IsKeyPressed(KEY_SPACE))
            {
                transitionRotateSpeed = 0.0f;
                cdSpinState = SPIN_STOPPING;
            }
            if (cdRotationSpeed >= cdDesiredRotationSpeed)
            {
                cdSpinState = SPIN_RUNNING;
            }
            break;

        case SPIN_RUNNING:
            // Uncomment line of code below for constant spinning cd
            // cdRotation += cdRotationSpeed;
            // Uncomment line of code above for constant spinning cd

            // Uncomment lines of code below for accelerating spinning cd
            if (accelState == ACCELERATING)
            {
                cdAcceleration += cdAccelerationSpeed;
                if (cdAcceleration >= 720)
                    accelState = DECELERATING;
            }
            else
            {
                cdAcceleration -= cdAccelerationSpeed;
                if (cdAcceleration <= 1)
                    accelState = ACCELERATING;
            }
            cdRotation += cdAcceleration;
            // Uncomment lines of code above for accelerating spinning cd

            // cdOverlayRotation logic. Chooses a random angle between 0 and 720. Moves to it with ease out
            if (cdOverlayRotation > cdOverlayRotationGoal + 0.2)
                cdOverlayRotation -= std::max(cdOverlaySpeed * 0.001f * (cdOverlayRotation - cdOverlayRotationGoal), cdOverlaySpeed * 0.01f);
            else if (cdOverlayRotation < cdOverlayRotationGoal - 0.2)
                cdOverlayRotation += std::max(cdOverlaySpeed * 0.001f * (cdOverlayRotationGoal - cdOverlayRotation), cdOverlaySpeed * 0.01f);
            else
                cdOverlayRotationGoal = rand() % 720;

            if (IsKeyPressed(KEY_SPACE))
            {
                cdSpinState = SPIN_STOPPING;
                transitionRotateSpeed = 0.0f;
            }
            break;

        case SPIN_STOPPING:

            // decelerating cdRotationSpeed
            transitionRotateSpeed += 0.0001f;
            cdRotationSpeed -= transitionRotateSpeed;
            cdRotation += cdRotationSpeed;

            cdOverlayRotation -= cdOverlaySpeed * 0.001f * (cdOverlayRotation - cdOverlayRotationGoal) * cdRotationSpeed * 0.03f;

            if (IsKeyPressed(KEY_SPACE))
            {
                transitionRotateSpeed = 0.0f;
                cdSpinState = SPIN_STARTING;
            }
            if (cdRotationSpeed <= 0.0f)
            {
                cdRotationSpeed = 0.0f;
                cdSpinState = SPIN_STOPPED;
            }
            break;
        }

        // since cdRotation is an angle between 0 and 360, there's no need for values above it
        cdRotation = fmod(cdRotation, 360.0);

        // Used for window dragging part 2
        Vector2 mousePos = GetMousePosition();
        Vector2 currentWindowPos = GetWindowPosition();

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
        {
            dragging = true;
            // Store offset between mouse and window's top-left corner
            dragOffset.x = mousePos.x;
            dragOffset.y = mousePos.y;
        }

        // When left mouse button is released, stop dragging
        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON))
        {
            dragging = false;
        }

        // Move window if dragging
        if (dragging)
        {
            Vector2 newMousePos = GetMousePosition();
            // SetWindowPosition(std::max(1920 - (int)windowWidth, std::min((int)currentWindowPos.x + (int)(newMousePos.x - dragOffset.x), 5000)),
            // currentWindowPos.y + (newMousePos.y - dragOffset.y));
            SetWindowPosition(currentWindowPos.x + newMousePos.x - dragOffset.x,
                              currentWindowPos.y + newMousePos.y - dragOffset.y);
        }

        // Updating

        // Can you tell this code can be improved? :,3
        // Hint: There's nothing here, also eveything is in this single file

        // Drawing
        BeginDrawing();

        // If we first render texture A and then texture B, texture B will be on top of A. Using this, we achieve layers

        // cdPlayerBottomShadow
        DrawTexturePro(windowBgTexture,
                       (Rectangle){0, 0, windowBgTexture.width, windowBgTexture.height}, // SOURCE
                       (Rectangle){0, 0, windowWidth, windowHeight},                     // DEST
                       (Vector2){0, 0},                                                  // ORIGIN
                       0,                                                                // ROTATION
                       WHITE);                                                           // idk

        // cdPlayerBottomShadow
        DrawTexturePro(cdPlayerBottomShadowTexture,
                       (Rectangle){0, 0, cdPlayerBottomTexture.width, cdPlayerBottomTexture.height},                // SOURCE
                       (Rectangle){windowWidth - 10, windowHeight / 2.0f + 10, cdPlayerWidth + 20, cdPlayerHeight}, // DEST
                       (Vector2){cdPlayerWidth, cdPlayerHeight / 2.0f},                                             // ORIGIN
                       0,                                                                                           // ROTATION
                       WHITE);                                                                                      // idk

        // cdPlayerBottom
        DrawTexturePro(cdPlayerBottomTexture,
                       (Rectangle){0, 0, cdPlayerBottomTexture.width, cdPlayerBottomTexture.height}, // SOURCE
                       (Rectangle){windowWidth, windowHeight / 2.0f, cdPlayerWidth, cdPlayerHeight}, // DEST
                       (Vector2){cdPlayerWidth, cdPlayerHeight / 2.0f},                              // ORIGIN
                       0,                                                                            // ROTATION
                       WHITE);                                                                       // idk

        // cdPlayerBottomNotScaled
        DrawTexturePro(cdPlayerBottomNotScaledTexture,
                       (Rectangle){0, 0, cdPlayerBottomNotScaledTexture.width, cdPlayerBottomNotScaledTexture.height}, // SOURCE
                       (Rectangle){windowWidth, windowHeight / 2.0f, cdPlayerWidth, cdPlayerHeight * cdPlayerScale},   // DEST
                       (Vector2){cdPlayerWidth, cdPlayerHeight * cdPlayerScale / 2.0f},                                // ORIGIN
                       0,                                                                                              // ROTATION
                       WHITE);                                                                                         // idk

        // cd
        DrawTexturePro(cdClearTexture,
                       (Rectangle){0, 0, cdClearTexture.width, cdClearTexture.height},                 // SOURCE
                       (Rectangle){cdSize / 2 + cdSize * 0.14f + 4, windowHeight / 2, cdSize, cdSize}, // DEST
                       (Vector2){cdSize / 2.0f, cdSize / 2.0f},                                        // ORIGIN
                       cdRotation * refreshRateMultiplier,                                             // ROTATION
                       WHITE);                                                                         // idk

        // if(cdRotationSpeed < 30)
        //     DrawTexturePro(cdClearTexture,
        //     (Rectangle){0, 0, cdClearTexture.width, cdClearTexture.height}, //SOURCE
        //     (Rectangle){cdSize/2 + cdSize * 0.14f + 4, windowHeight/2, cdSize, cdSize}, //DEST
        //     (Vector2){cdSize/2.0f, cdSize/2.0f}, //ORIGIN
        //     cdRotation * refreshRateMultiplier, //ROTATION
        //     WHITE); //idk
        // else
        //     DrawTexturePro(cdBlurTexture,
        //     (Rectangle){0, 0, cdBlurTexture.width, cdBlurTexture.height}, //SOURCE
        //     (Rectangle){cdSize/2 + cdSize * 0.14f + 4, windowHeight/2, cdSize, cdSize}, //DEST
        //     (Vector2){cdSize/2.0f, cdSize/2.0f}, //ORIGIN
        //     cdRotation * refreshRateMultiplier, //ROTATION
        //     WHITE); //idk

        // cdNipple
        DrawTexturePro(cdNippleTexture,
                       (Rectangle){0, 0, cdNippleTexture.width, cdNippleTexture.height},                                                                         // SOURCE
                       (Rectangle){cdNippleSize / 2 + cdSize * 0.49f + 4 - 155 * (cdNippleSize / cdSize) + 47.5f, windowHeight / 2, cdNippleSize, cdNippleSize}, // DEST
                       (Vector2){cdNippleSize / 2.0f, cdNippleSize / 2.0f},                                                                                      // ORIGIN
                       cdRotation * refreshRateMultiplier,                                                                                                       // ROTATION
                       WHITE);                                                                                                                                   // idk

        BeginBlendMode(cdOverlayBlendMode);
        // cdOverlay
        DrawTexturePro(cdOverlayTexture,
                       (Rectangle){0, 0, cdOverlayTexture.width, cdOverlayTexture.height},             // SOURCE
                       (Rectangle){cdSize / 2 + cdSize * 0.14f + 4, windowHeight / 2, cdSize, cdSize}, // DEST
                       (Vector2){cdSize / 2.0f, cdSize / 2.0f},                                        // ORIGIN
                       cdOverlayRotation * refreshRateMultiplier,                                      // ROTATION
                       WHITE);                                                                         // idk
        EndBlendMode();

        // cdPlayerTop
        DrawTexturePro(cdPlayerTopTexture,
                       (Rectangle){0, 0, cdPlayerTopTexture.width, cdPlayerTopTexture.height},                       // SOURCE
                       (Rectangle){windowWidth, windowHeight / 2.0f, cdPlayerWidth, cdPlayerHeight * cdPlayerScale}, // DEST
                       (Vector2){cdPlayerWidth, cdPlayerHeight * cdPlayerScale / 2.0f},                              // ORIGIN
                       0,                                                                                            // ROTATION
                       WHITE);                                                                                       // idk

        // Funky formula with the goal of positioning the FPS counter inside the cd player, regardless of cdSize and cdPlayerScale
        DrawFPS(windowWidth - 110, windowHeight - cdSize - 20 + 100 * cdPlayerScale - 100);

        EndDrawing();
    }

    CloseWindow();
}