#pragma once

#include "script.h"
#include <vector>

// Defines a single vehicle that can be requested for export.
// The 'base_reward' is the payout for a car delivered in perfect condition.
struct ExportableVehicle {
    const char* name;
    Hash hash;
    int base_reward;
};

// Declares the core functions for the Car Export system.
// These will be called from your main script.cpp file.
void CarExport_Init();  // Sets up the initial state and blips.
void CarExport_Tick();  // Runs every frame to handle mission logic.
void CarExport_Draw();  // Draws on-screen text for the player.
