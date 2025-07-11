#define _CRT_SECURE_NO_WARNINGS
#include "script.h"
#include "Vehicle.h"
#include "RankBar.h"
#include "CharacterCreator.h"
#include "RpEvents.h"
#include "Money.h"
#include "Cheats.h"
#include "input.h"
#include "CarShop.h"
#include "Garage.h"
#include "GunStore.h"
#include "Credits.h" // Ensure Credits.h is included for Credits_Tick()
#include "CarExport.h"
#include <windows.h>
#include <ctime>
#include <cstdio>
#include <vector>
#include <algorithm>

// Defensive macros
#define VALID_PED(ped)     ((ped) != 0 && ENTITY::DOES_ENTITY_EXIST(ped))
#define VALID_PLAYER(p)   ((p) != -1 && PLAYER::IS_PLAYER_PLAYING(p))

#pragma warning(disable : 4244 4305)

// --- File Constants ---
const char* characterFile = "GTAOfflineChar.ini";
const char* playerStatsFile = "GTAOfflinePlayerStats.ini";
const char* xpFile = "GTAOfflineXP.ini";

// --- Menu State ---
enum Category {
    CAT_MAIN = 0, CAT_CHARACTER, CAT_CHEATS, CAT_VEHICLE,
    CAT_SAVELOAD, CAT_CAR_SHOP, CAT_GARAGE, CAT_GUN_STORE, CAT_CREDITS
};
int menuCategory = CAT_MAIN;
int menuIndex = 0;
bool menuOpen = false;
int inputDelayFrames = 0;

// --- UI Constants ---
const float MENU_X = 0.02f;
const float MENU_Y = 0.13f; // This is now the Y coordinate for the TOP of the menu block
const float MENU_W = 0.29f;
const float MENU_H = 0.038f; // Height of a single option/header row

// --- UI Color Palette Definition ---
const int FONT = 0;
const RGBA BG_COLOR = { 20, 20, 20, 200 };
const RGBA HEADER_COLOR = { 30, 144, 255, 220 };
const RGBA OPTION_COLOR = { 40, 40, 40, 200 };
const RGBA SELECTED_COLOR = { 255, 255, 255, 220 };
const RGBA TEXT_COLOR = { 255, 255, 255, 255 };
const RGBA SELECTED_TEXT_COLOR = { 0, 0, 0, 255 };
const RGBA HEADER_TEXT_COLOR = { 255, 255, 255, 255 };
const RGBA TAB_BG_COLOR = { 40, 40, 40, 200 };
const RGBA SELECTED_TAB_COLOR = { 50, 160, 255, 220 };


// --- Spawn State ---
static bool spawnProtectionActive = true;
static DWORD spawnTime = 0;
static bool welcomeMessagesShown = false;
static bool isCustomCharacterActive = true; // Track if the custom character is currently applied

// Forward declarations
void draw_car_shop_menu();
void draw_garage_menu();
void draw_gun_store_menu();
void draw_credits_menu();
void LoadGameData();


// --- Drawing Helper Function Definitions ---
// Restored DrawMenuHeader function
void DrawMenuHeader(const char* text, float x, float y, float w) {
    // This function now draws only the text, the background is handled by the caller
    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.43f);
    UI::SET_TEXT_COLOUR(HEADER_TEXT_COLOR.r, HEADER_TEXT_COLOR.g, HEADER_TEXT_COLOR.b, HEADER_TEXT_COLOR.a);
    UI::SET_TEXT_CENTRE(true);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text));
    UI::_DRAW_TEXT(x + w * 0.5f, y + 0.007f); // Position text relative to the top of its row
}

void DrawMenuOption(const char* text, float x, float y, float w, float h, bool selected) {
    GRAPHICS::DRAW_RECT(x + w * 0.5f, y + (h - 0.004f) * 0.5f, w, h - 0.004f,
        selected ? SELECTED_COLOR.r : OPTION_COLOR.r,
        selected ? SELECTED_COLOR.g : OPTION_COLOR.g,
        selected ? SELECTED_COLOR.b : OPTION_COLOR.b,
        selected ? SELECTED_COLOR.a : OPTION_COLOR.a);
    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.38f);
    UI::SET_TEXT_COLOUR(
        selected ? SELECTED_TEXT_COLOR.r : TEXT_COLOR.r,
        selected ? SELECTED_TEXT_COLOR.g : TEXT_COLOR.g,
        selected ? SELECTED_TEXT_COLOR.b : TEXT_COLOR.b,
        selected ? SELECTED_TEXT_COLOR.a : TEXT_COLOR.a);
    UI::SET_TEXT_CENTRE(false);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(text));
    UI::_DRAW_TEXT(x + 0.01f, y + 0.007f);
}

void DrawPairedMenuOption(const char* label, const char* value, float x, float y, float w, float h, bool selected) {
    DrawMenuOption(label, x, y, w, h, selected);

    UI::SET_TEXT_FONT(FONT);
    UI::SET_TEXT_SCALE(0.0f, 0.38f);
    UI::SET_TEXT_COLOUR(
        selected ? SELECTED_TEXT_COLOR.r : TEXT_COLOR.r,
        selected ? SELECTED_TEXT_COLOR.g : TEXT_COLOR.g,
        selected ? SELECTED_TEXT_COLOR.b : TEXT_COLOR.b,
        selected ? SELECTED_TEXT_COLOR.a : TEXT_COLOR.a);
    UI::SET_TEXT_RIGHT_JUSTIFY(true);
    UI::SET_TEXT_WRAP(0.0f, x + w - 0.01f);
    UI::_SET_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING(const_cast<char*>(value));
    UI::_DRAW_TEXT(x, y + 0.007f);
    UI::SET_TEXT_RIGHT_JUSTIFY(false);
}

void draw_main_menu() {
    const int numOptions = 9;
    const char* labels[numOptions] = {
        "Character Creator", "Cheats", "Vehicle", "Save/Load",
        "Car Shop", "Garage", "Gun Store", "Credits", "Close Menu"
    };

    float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;

    // Calculate total height including header
    float headerHeight = MENU_H; // Height of the header bar
    float optionsTotalHeight = h * numOptions; // Total height of all options
    float totalMenuHeight = headerHeight + optionsTotalHeight;

    // Calculate the center Y for the entire menu block (header + options)
    // The top of the menu is MENU_Y. The center of the entire block will be MENU_Y + (totalMenuHeight / 2.0f)
    float menuBlockCenterY = MENU_Y + (totalMenuHeight / 2.0f);

    // Draw ONE unified background for the entire menu (header + options)
    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    // Draw the header background (blue part)
    float headerBgCenterY = MENU_Y + (headerHeight * 0.5f); // Center Y of the header bar
    GRAPHICS::DRAW_RECT(x + w * 0.5f, headerBgCenterY, w, headerHeight, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);

    // Draw the header text using the shared function
    DrawMenuHeader("GTA OFFLINE", x, MENU_Y, w); // Pass MENU_Y as the top of the header row

    // Draw menu options, starting below the header
    float optionsStartY = MENU_Y + headerHeight;
    for (int i = 0; i < numOptions; ++i) {
        DrawMenuOption(labels[i], x, optionsStartY + h * i, w, h, i == menuIndex);
    }

    ClampMenuIndex(menuIndex, numOptions);

    if (inputDelayFrames > 0) return;

    int up = 0, down = 0;
    if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
    if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;
    if (up)   menuIndex = (menuIndex - 1 + numOptions) % numOptions;
    if (down) menuIndex = (menuIndex + 1) % numOptions;

    static bool prevA = false;
    bool currA = PadPressed(BTN_A);
    if ((IsKeyJustUp(VK_NUMPAD5) || (currA && !prevA))) {
        switch (menuIndex) {
        case 0: menuCategory = CAT_CHARACTER; menuIndex = 0; inputDelayFrames = 10; break;
        case 1: menuCategory = CAT_CHEATS;    menuIndex = 0; inputDelayFrames = 10; break;
        case 2: menuCategory = CAT_VEHICLE;   menuIndex = 0; inputDelayFrames = 10; break;
        case 3: menuCategory = CAT_SAVELOAD;  menuIndex = 0; inputDelayFrames = 10; break;
        case 4: menuCategory = CAT_CAR_SHOP;  menuIndex = 0; inputDelayFrames = 10; break;
        case 5: menuCategory = CAT_GARAGE;    menuIndex = 0; inputDelayFrames = 10; break;
        case 6: menuCategory = CAT_GUN_STORE; menuIndex = 0; inputDelayFrames = 10; break;
        case 7: menuCategory = CAT_CREDITS;   menuIndex = 0; inputDelayFrames = 10; break;
        case 8: menuOpen = false;             menuIndex = 0; inputDelayFrames = 10; break;
        }
    }
    prevA = currA;
}


int saveloadMenuIndex = 0;
void draw_saveload_menu() {
    const int numOptions = 3;
    const char* labels[numOptions] = { "Save Game", "Load Game", "Back" };

    float x = MENU_X, y = MENU_Y, w = MENU_W, h = MENU_H;
    // Calculate total height including header
    float headerHeight = MENU_H; // Height of the header bar
    float optionsTotalHeight = h * numOptions; // Total height of all options
    float totalMenuHeight = headerHeight + optionsTotalHeight;

    // Calculate the center Y for the entire menu block (header + options)
    float menuBlockCenterY = MENU_Y + (totalMenuHeight / 2.0f);

    GRAPHICS::DRAW_RECT(x + w * 0.5f, menuBlockCenterY, w, totalMenuHeight, BG_COLOR.r, BG_COLOR.g, BG_COLOR.b, BG_COLOR.a);

    // Draw the header background (blue part)
    float headerBgCenterY = MENU_Y + (headerHeight * 0.5f); // Center Y of the header bar
    GRAPHICS::DRAW_RECT(x + w * 0.5f, headerBgCenterY, w, headerHeight, HEADER_COLOR.r, HEADER_COLOR.g, HEADER_COLOR.b, HEADER_COLOR.a);

    // Draw the header text using the shared function
    DrawMenuHeader("SAVE / LOAD", x, MENU_Y, w); // Pass MENU_Y as the top of the header row

    // Draw menu options, starting below the header
    float optionsStartY = MENU_Y + headerHeight;
    for (int i = 0; i < numOptions; ++i) {
        DrawMenuOption(labels[i], x, optionsStartY + h * i, w, h, i == saveloadMenuIndex);
    }

    ClampMenuIndex(saveloadMenuIndex, numOptions);

    if (inputDelayFrames > 0) return;

    int up = 0, down = 0;
    if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
    if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;
    if (up)   saveloadMenuIndex = (saveloadMenuIndex - 1 + numOptions) % numOptions;
    if (down) saveloadMenuIndex = (saveloadMenuIndex + 1) % numOptions;

    static bool prevA = false;
    bool currA = PadPressed(BTN_A);
    if ((IsKeyJustUp(VK_NUMPAD5) || (currA && !prevA))) {
        if (saveloadMenuIndex == 0) {
            CharacterCreator_Save(characterFile);
            Money_Save(playerStatsFile);
            RankBar_Save(playerStatsFile);
            RpEvents_Save(xpFile);
            GunStore_Save();

            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("Game Saved.");
            UI::_DRAW_NOTIFICATION(false, true);
        }
        else if (saveloadMenuIndex == 1) {
            LoadGameData();

            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("Game Loaded.");
            UI::_DRAW_NOTIFICATION(false, true);
        }
        else if (saveloadMenuIndex == 2) {
            menuCategory = CAT_MAIN; menuIndex = 3; saveloadMenuIndex = 0; inputDelayFrames = 10;
        }
    }
    prevA = currA;
}

void LoadGameData()
{
    CharacterCreator_Load(characterFile);
    RankBar_Load(characterFile);
    Money_Load(characterFile);
    WAIT(100);

    RankBar_Load(playerStatsFile);
    Money_Load(playerStatsFile);
    WAIT(100);

    RpEvents_Load(xpFile);
    GunStore_Load();
    WAIT(100);

    CharacterCreator_Apply();
    isCustomCharacterActive = true; // Mark custom character as active after loading
}


void SkipIntroAndLoadCharacter()
{
    spawnTime = GetTickCount64();

    CAM::DO_SCREEN_FADE_OUT(0);
    WAIT(700);

    NETWORK::NETWORK_END_TUTORIAL_SESSION();

    Player player = PLAYER::PLAYER_ID();
    Ped playerPed = PLAYER::PLAYER_PED_ID();
    ENTITY::FREEZE_ENTITY_POSITION(playerPed, false);

    WAIT(1200);

    while (!PLAYER::IS_PLAYER_PLAYING(player)) WAIT(100);

    PLAYER::SET_PLAYER_CONTROL(player, true, 0);
    WAIT(400);

    CAM::DO_SCREEN_FADE_IN(800);

    WAIT(1000);

    LoadGameData();
}

void ScriptMain() {
    Cheats_Init();
    RankBar_Init();
    Money_Init();
    CharacterCreator_Init();
    RpEvents_Init();
    CarShop_Init();
    Garage_Init();
    GunStore_Init();
    Credits_Init(); // Initialize Credits module
    CarExport_Init();

    SkipIntroAndLoadCharacter();

    extern void (*Vehicle_DrawMenu)(int& menuIndex, int& menuCategory);
    srand((unsigned int)GetTickCount64());

    bool prevMenuCombo = false;
    bool prevB = false;

    while (true)
    {
        PollPad();

        Player player = PLAYER::PLAYER_ID();
        Ped playerPed = PLAYER::PLAYER_PED_ID();

        // --- Custom Character Respawn Logic ---
        if (PLAYER::IS_PLAYER_DEAD(player)) {
            if (isCustomCharacterActive) {
                // If custom character is active and player dies, switch to a default model for respawn
                isCustomCharacterActive = false; // Temporarily mark as not custom
                PLAYER::SET_PLAYER_MODEL(player, GAMEPLAY::GET_HASH_KEY("player_g")); // Switch to a generic ped model for stable respawn

                // Ensure a clean fade and respawn
                GAMEPLAY::SET_FADE_OUT_AFTER_DEATH(false); // Don't fade out again if already dead
                GAMEPLAY::SET_FADE_IN_AFTER_DEATH_ARREST(true); // Ensure fade in after respawn

                // Force respawn and clear wanted level
                PLAYER::SET_PLAYER_CONTROL(player, true, 0);
                PLAYER::SET_PLAYER_WANTED_LEVEL(player, 0, false);
                PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, true);

                // Wait for respawn to complete
                while (!PLAYER::IS_PLAYER_PLAYING(player)) {
                    WAIT(0);
                }

                // After respawn, re-apply the custom character
                CharacterCreator_Apply();
                isCustomCharacterActive = true; // Mark custom character as active again
                spawnProtectionActive = true; // Re-enable spawn protection after respawn
                spawnTime = GetTickCount64(); // Reset spawn time for protection
            }
        }
        // --- End Custom Character Respawn Logic ---


        if (spawnProtectionActive) {
            ENTITY::SET_ENTITY_INVINCIBLE(playerPed, true);
            if (GetTickCount64() - spawnTime > 15000) {
                ENTITY::SET_ENTITY_INVINCIBLE(playerPed, false);
                spawnProtectionActive = false;
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("Spawn protection has worn off.");
                UI::_DRAW_NOTIFICATION(false, true);
            }
        }

        if (!welcomeMessagesShown && GetTickCount64() - spawnTime > 2000) {
            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("~b~GTA Offline by CreamyPlaytime Loaded");
            UI::_DRAW_NOTIFICATION(false, true);

            WAIT(4000);

            UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
            UI::_ADD_TEXT_COMPONENT_STRING("Press F4 (Keyboard) or RB + A (Controller) to open the menu.");
            UI::_DRAW_NOTIFICATION(false, true);

            welcomeMessagesShown = true;
        }

        Cheats_Tick();
        RpEvents_Tick();
        RankBar_Tick();
        CharacterCreator_Tick();
        CarShop_Tick();
        Garage_Tick();
        GunStore_Tick();
        CarExport_Tick();

        g_vehicleMenu.Tick();

        Money_PickupScan();

        bool menuCombo = PadHeld(BTN_RB) && PadHeld(BTN_A);

        if (!menuOpen && (IsKeyJustUp(VK_F4) || (menuCombo && !prevMenuCombo))) {
            menuOpen = true;
            menuIndex = 0;
            menuCategory = CAT_MAIN;
            inputDelayFrames = 15;
        }
        prevMenuCombo = menuCombo;

        bool currB = PadPressed(BTN_B);
        if (menuOpen && inputDelayFrames == 0 && ((currB && !prevB) || IsKeyJustUp(VK_NUMPAD0))) {
            if (menuCategory == CAT_MAIN) {
                menuOpen = false;
                menuIndex = 0;
                inputDelayFrames = 10;
            }
            else {
                menuCategory = CAT_MAIN;
                menuIndex = 0;
                inputDelayFrames = 10;
            }
        }
        prevB = currB;

        if (menuOpen)
        {
            CONTROLS::DISABLE_CONTROL_ACTION(0, 27, true);
            switch (menuCategory) {
            case CAT_MAIN:
                draw_main_menu();
                break;
            case CAT_CHARACTER:
                CharacterCreator_DrawMenu(menuIndex, menuCategory);
                break;
            case CAT_CHEATS:
                Cheats_DrawMenu(menuIndex, MENU_X, MENU_Y, MENU_W, MENU_H);
                break;
            case CAT_VEHICLE:
                Vehicle_DrawMenu(menuIndex, menuCategory);
                break;
            case CAT_SAVELOAD:
                draw_saveload_menu();
                break;
            case CAT_CAR_SHOP:
                draw_car_shop_menu();
                break;
            case CAT_GARAGE:
                draw_garage_menu();
                break;
            case CAT_GUN_STORE:
                draw_gun_store_menu();
                break;
            case CAT_CREDITS:
                draw_credits_menu();
                break;
            }
        }
        else {
            CarExport_Draw();
        }

        RankBar_DrawBar();
        Money_Draw();

        if (inputDelayFrames > 0) inputDelayFrames--;

        WAIT(0);
    }
}
