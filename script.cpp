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
#include "GunStore.h" // Include GunStore.h for the new function
#include "Credits.h"
#include "CarExport.h"
#include "Properties.h" // Ensure Properties.h is included
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
const char* propertiesFile = "GTAOfflineProperties.ini"; // Define properties file name

// --- Game Logic Constants ---
const int BRIBE_AMOUNT = 5000; // Define the bribe amount here

// --- Menu State ---
enum Category {
    CAT_MAIN = 0, CAT_CHARACTER, CAT_CHEATS, CAT_VEHICLE,
    CAT_SAVELOAD, CAT_CAR_SHOP, CAT_GARAGE, CAT_GUN_STORE, CAT_CREDITS
};
int menuCategory = CAT_MAIN;
int menuIndex = 0;
bool menuOpen = false;
int inputDelayFrames = 0; // Global input delay for menu navigation/selection

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

// State for handling the "busted" event
enum BustedState {
    BustedState_None = 0,
    BustedState_Initiated,
    BustedState_Recovering
};
static BustedState currentBustedState = BustedState_None;


// Forward declarations
void draw_car_shop_menu();
void draw_garage_menu();
void draw_gun_store_menu();
void draw_credits_menu();
void LoadGameData();

// Native function declarations
// These assume they are available through ScriptHookV's headers (e.g., natives.h)
// The hash 0x388A47C51ABDAC8E is used to invoke the native.
static BOOL IS_PLAYER_BEING_ARRESTED(Player player, BOOL atArresting) { return invoke<BOOL>(0x388A47C51ABDAC8E, player, atArresting); }
static BOOL IS_PED_CUFFED(Ped ped) { return invoke<BOOL>(0x74E559B3BC910685, ped); }
static void UNCUFF_PED(Ped ped) { invoke<Void>(0x67406F2C8F87FC4F, ped); } // Added UNCUFF_PED native

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

    // Only process input if the global input delay is 0
    if (inputDelayFrames == 0) {
        int up = 0, down = 0;
        if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
        if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

        if (up) {
            menuIndex = (menuIndex - 1 + numOptions) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }
        if (down) {
            menuIndex = (menuIndex + 1) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }

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

    // Only process input if the global input delay is 0
    if (inputDelayFrames == 0) {
        int up = 0, down = 0;
        if (IsKeyJustUp(VK_NUMPAD8) || PadPressed(DPAD_UP))   up = 1;
        if (IsKeyJustUp(VK_NUMPAD2) || PadPressed(DPAD_DOWN)) down = 1;

        if (up) {
            saveloadMenuIndex = (saveloadMenuIndex - 1 + numOptions) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }
        if (down) {
            saveloadMenuIndex = (saveloadMenuIndex + 1) % numOptions;
            inputDelayFrames = 10; // Apply delay after navigation
        }

        static bool prevA = false;
        bool currA = PadPressed(BTN_A);
        if ((IsKeyJustUp(VK_NUMPAD5) || (currA && !prevA))) {
            if (saveloadMenuIndex == 0) {
                CharacterCreator_Save(characterFile);
                Money_Save(playerStatsFile);
                RankBar_Save(playerStatsFile);
                RpEvents_Save(xpFile);
                GunStore_Save();
                Properties_Save(propertiesFile); // Save properties here

                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("Game Saved.");
                UI::_DRAW_NOTIFICATION(false, true);
                inputDelayFrames = 10; // Apply delay after action
            }
            else if (saveloadMenuIndex == 1) {
                LoadGameData();

                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("Game Loaded.");
                UI::_DRAW_NOTIFICATION(false, true);
                inputDelayFrames = 10; // Apply delay after action
            }
            else if (saveloadMenuIndex == 2) {
                menuCategory = CAT_MAIN; menuIndex = 3; saveloadMenuIndex = 0; inputDelayFrames = 10; // Apply delay after category change
            }
        }
        prevA = currA;
    }
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
    Properties_Load(propertiesFile); // Load properties here
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
    Credits_Init();
    CarExport_Init();
    Properties_Init();

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

        // --- Custom Character Respawn Logic (Death) ---
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
        // --- End Custom Character Respawn Logic (Death) ---

        // --- Custom Character Respawn Logic (Busted) ---
        bool currBeingArrested = IS_PLAYER_BEING_ARRESTED(player, true);
        bool currCuffed = IS_PED_CUFFED(playerPed);

        switch (currentBustedState) {
        case BustedState_None:
            // Detect if player just started being arrested and is not dead
            if (currBeingArrested && !PLAYER::IS_PLAYER_DEAD(player)) {
                if (isCustomCharacterActive) {
                    // If in a vehicle, warp out to ensure proper arrest animation/sequence
                    if (PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE)) {
                        Vehicle veh = PED::GET_VEHICLE_PED_IS_IN(playerPed, FALSE);
                        AI::TASK_LEAVE_VEHICLE(playerPed, veh, 16); // 16 = leave instantly
                        int waitTime = 0;
                        while (PED::IS_PED_IN_ANY_VEHICLE(playerPed, FALSE) && waitTime < 2000) {
                            WAIT(50);
                            waitTime += 50;
                        }
                    }

                    // Temporarily switch to generic model to allow game to handle arrest
                    isCustomCharacterActive = false;
                    PLAYER::SET_PLAYER_MODEL(player, GAMEPLAY::GET_HASH_KEY("player_g"));

                    // Ensure fade in after arrest
                    GAMEPLAY::SET_FADE_IN_AFTER_DEATH_ARREST(true);

                    // Clear wanted level immediately
                    PLAYER::SET_PLAYER_WANTED_LEVEL(player, 0, false);
                    PLAYER::SET_PLAYER_WANTED_LEVEL_NOW(player, true);
                    currentBustedState = BustedState_Initiated; // Transition to initiated state
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("~b~You are being busted...");
                    UI::_DRAW_NOTIFICATION(false, true);
                }
            }
            break;

        case BustedState_Initiated:
            // Wait for the player to no longer be actively arrested or cuffed, and screen to fade in
            // Also ensure the player is actually playing (i.e., not in a loading screen or transition)
            if (!currBeingArrested && !currCuffed && CAM::IS_SCREEN_FADED_IN() && PLAYER::IS_PLAYER_PLAYING(player)) {
                if (!isCustomCharacterActive) { // Only re-apply if it was temporarily disabled
                    // Ensure the player is uncuffed before applying character if they somehow got cuffed
                    if (IS_PED_CUFFED(playerPed)) {
                        UNCUFF_PED(playerPed);
                        WAIT(100); // Small delay after uncuffing
                    }

                    // Handle bribe or confiscation
                    if (Money_Get() >= BRIBE_AMOUNT) {
                        Money_Add(-BRIBE_AMOUNT); // Deduct bribe
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~g~Bribe paid! ($5000)");
                        UI::_DRAW_NOTIFICATION(false, true);
                    }
                    else {
                        GunStore_ClearAllBoughtWeapons(); // Clear all bought weapons from persistent storage
                        WEAPON::REMOVE_ALL_PED_WEAPONS(playerPed, false); // Remove all weapons from player's inventory
                        UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                        UI::_ADD_TEXT_COMPONENT_STRING("~r~Not enough money! All your bought guns were confiscated!");
                        UI::_DRAW_NOTIFICATION(false, true);
                    }

                    CharacterCreator_Apply();
                    isCustomCharacterActive = true; // Mark custom character as active again
                    spawnProtectionActive = true; // Re-enable spawn protection after being busted
                    spawnTime = GetTickCount64(); // Reset spawn time for protection
                    currentBustedState = BustedState_None; // Reset state to None
                }
            }
            // If for some reason the player died during the arrest, reset state
            if (PLAYER::IS_PLAYER_DEAD(player)) {
                currentBustedState = BustedState_None;
            }
            break;
        }
        // --- End Custom Character Respawn Logic (Busted) ---


        if (spawnProtectionActive) {
            ENTITY::SET_ENTITY_INVINCIBLE(playerPed, true);
            if (GetTickCount64() - spawnTime > 15000) {
                ENTITY::SET_ENTITY_INVINCIBLE(playerPed, false);
                UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                UI::_ADD_TEXT_COMPONENT_STRING("Spawn protection has worn off.");
                UI::_DRAW_NOTIFICATION(false, true);
                spawnProtectionActive = false; // Set to false after notification
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
        Properties_Tick();

        g_vehicleMenu.Tick();

        Money_PickupScan();

        bool menuCombo = PadHeld(BTN_RB) && PadHeld(BTN_A);

        // Menu Open/Close Logic (Consolidated Toggle)
        if (inputDelayFrames == 0) { // Only process menu open/close if no global input delay
            if (IsKeyJustUp(VK_F4) || (menuCombo && !prevMenuCombo)) {
                menuOpen = !menuOpen; // Toggle menu state
                menuIndex = 0; // Reset index when toggling
                menuCategory = CAT_MAIN; // Always go back to main category when toggling
                inputDelayFrames = 15; // Apply delay after toggling
            }

            // This logic is for going back within sub-menus or closing from main using B/Numpad0
            bool currB = PadPressed(BTN_B);
            if (menuOpen && ((currB && !prevB) || IsKeyJustUp(VK_NUMPAD0))) {
                if (menuCategory == CAT_MAIN) {
                    menuOpen = false;
                    menuIndex = 0;
                    inputDelayFrames = 10; // Apply delay after closing
                }
                else {
                    menuCategory = CAT_MAIN;
                    menuIndex = 0;
                    inputDelayFrames = 10; // Apply delay after category change
                }
            }
            prevB = currB;
        }


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

        // Decrement input delay frames at the end of the tick
        if (inputDelayFrames > 0) inputDelayFrames--;

        WAIT(0);
    }
}
