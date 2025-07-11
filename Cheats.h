#pragma once

// Defines the total number of tabs in the cheat menu.
#define TAB_COUNT 3

// Initializes the cheat menu and its sub-modules.
void Cheats_Init();

// Updates the logic for the cheat menu on each game tick.
void Cheats_Tick();

// Draws the cheat menu, including the header, tabs, and options for the active tab.
void Cheats_DrawMenu(int& menuIndex, float x, float y, float w, float h);

// --- Forward declarations for submodule functions ---
// These are needed by Cheats_DrawMenu to get the option count for proper background drawing.
int Self_GetNumOptions();
int Weapons_GetNumOptions();
int Misc_GetNumOptions();

// These are the full declarations for each submodule.
void Self_Init();
void Self_Tick();
void Self_DrawMenu(int& menuIndex, float x, float y, float w, float h);

void Weapons_Init();
void Weapons_Tick();
void Weapons_DrawMenu(int& menuIndex, float x, float y, float w, float h);

void Misc_Init();
void Misc_Tick();
void Misc_DrawMenu(int& menuIndex, float x, float y, float w, float h);
