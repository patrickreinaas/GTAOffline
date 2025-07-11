#include "CarExport.h"
#include "Money.h"
#include "RpEvents.h" 
#include "script.h"
#include "input.h"
#include "Garage.h" 
#include <vector>
#include <algorithm>

// --- Mission Settings & State ---
static std::vector<ExportableVehicle> g_exportable_vehicles;
static int g_active_mission_index = -1;
static Blip g_mission_blip = 0;
static ULONGLONG g_mission_cooldown_end_time = 0;
const DWORD MISSION_COOLDOWN_MS = 30000;

// --- Delivery State Variables ---
static bool g_can_sell_now = false;
static int g_potential_payout = 0;

// --- Locations ---
static Vector3 g_dropoff_location = { -357.0f, -134.0f, 39.0f };


// --- Helper Function to handle mission failure ---
void fail_mission(Vehicle* vehicle_handle) {
    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
    UI::_ADD_TEXT_COMPONENT_STRING("~r~Mission Failed.");
    UI::_DRAW_NOTIFICATION(false, true);

    if (UI::DOES_BLIP_EXIST(g_mission_blip)) {
        UI::REMOVE_BLIP(&g_mission_blip);
    }

    if (vehicle_handle != NULL && ENTITY::DOES_ENTITY_EXIST(*vehicle_handle)) {
        ENTITY::SET_ENTITY_AS_MISSION_ENTITY(*vehicle_handle, false, true);
    }

    g_active_mission_index = -1;
    g_mission_blip = 0;
    g_can_sell_now = false;
    g_mission_cooldown_end_time = GetTickCount64() + MISSION_COOLDOWN_MS;
}


void CarExport_Init() {
    g_exportable_vehicles.push_back({ "Sultan", GAMEPLAY::GET_HASH_KEY("SULTAN"), 12000 });
    g_exportable_vehicles.push_back({ "Sabre Turbo", GAMEPLAY::GET_HASH_KEY("SABREGT"), 15000 });
    g_exportable_vehicles.push_back({ "Ruiner", GAMEPLAY::GET_HASH_KEY("RUINER"), 10000 });
    g_exportable_vehicles.push_back({ "Vigero", GAMEPLAY::GET_HASH_KEY("VIGERO"), 18000 });
    g_exportable_vehicles.push_back({ "Manana", GAMEPLAY::GET_HASH_KEY("MANANA"), 7000 });
    g_exportable_vehicles.push_back({ "Youga", GAMEPLAY::GET_HASH_KEY("YOUGA"), 13000 });
    g_exportable_vehicles.push_back({ "Feltzer", GAMEPLAY::GET_HASH_KEY("FELTZER2"), 28000 });
    g_exportable_vehicles.push_back({ "Comet", GAMEPLAY::GET_HASH_KEY("COMET2"), 25000 });
    g_exportable_vehicles.push_back({ "9F", GAMEPLAY::GET_HASH_KEY("NINEF"), 29000 });
    g_exportable_vehicles.push_back({ "Schafter", GAMEPLAY::GET_HASH_KEY("SCHAFTER2"), 32000 });
    g_exportable_vehicles.push_back({ "Zion", GAMEPLAY::GET_HASH_KEY("ZION"), 26000 });
    g_exportable_vehicles.push_back({ "Dominator", GAMEPLAY::GET_HASH_KEY("DOMINATOR"), 25000 });
    g_exportable_vehicles.push_back({ "F620", GAMEPLAY::GET_HASH_KEY("F620"), 22000 });
    g_exportable_vehicles.push_back({ "Dubsta", GAMEPLAY::GET_HASH_KEY("DUBSTA"), 35000 });
    g_exportable_vehicles.push_back({ "Sandking XL", GAMEPLAY::GET_HASH_KEY("SANDKING"), 38000 });
    g_exportable_vehicles.push_back({ "Jester", GAMEPLAY::GET_HASH_KEY("JESTER"), 60000 });
    g_exportable_vehicles.push_back({ "Massacro", GAMEPLAY::GET_HASH_KEY("MASSACRO"), 75000 });
    g_exportable_vehicles.push_back({ "Carbonizzare", GAMEPLAY::GET_HASH_KEY("CARBONIZZARE"), 80000 });
    g_exportable_vehicles.push_back({ "Coquette Classic", GAMEPLAY::GET_HASH_KEY("COQUETTE2"), 55000 });
    g_exportable_vehicles.push_back({ "Huntley S", GAMEPLAY::GET_HASH_KEY("HUNTLEY"), 90000 });
    g_exportable_vehicles.push_back({ "Vacca", GAMEPLAY::GET_HASH_KEY("VACCA"), 120000 });
    g_exportable_vehicles.push_back({ "Infernus", GAMEPLAY::GET_HASH_KEY("INFERNUS"), 150000 });
    g_exportable_vehicles.push_back({ "Cheetah", GAMEPLAY::GET_HASH_KEY("CHEETAH"), 180000 });
}

void CarExport_Tick() {
    Ped player_ped = PLAYER::PLAYER_PED_ID();

    if (g_active_mission_index == -1) { // --- NO MISSION ACTIVE ---
        if (GetTickCount64() < g_mission_cooldown_end_time) {
            return;
        }

        if (PED::IS_PED_IN_ANY_VEHICLE(player_ped, false)) {
            Vehicle current_vehicle = PED::GET_VEHICLE_PED_IS_IN(player_ped, false);

            if (Garage_IsVehicleOwned(current_vehicle)) {
                return;
            }

            Hash vehicle_hash = ENTITY::GET_ENTITY_MODEL(current_vehicle);

            for (size_t i = 0; i < g_exportable_vehicles.size(); ++i) {
                if (g_exportable_vehicles[i].hash == vehicle_hash) {
                    g_active_mission_index = i;

                    ENTITY::SET_ENTITY_AS_MISSION_ENTITY(current_vehicle, true, true);

                    g_mission_blip = UI::ADD_BLIP_FOR_COORD(g_dropoff_location.x, g_dropoff_location.y, g_dropoff_location.z);
                    UI::SET_BLIP_SPRITE(g_mission_blip, 357);
                    UI::SET_BLIP_COLOUR(g_mission_blip, 5);
                    UI::SET_BLIP_ROUTE(g_mission_blip, true);
                    UI::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING("Vehicle Export");
                    UI::END_TEXT_COMMAND_SET_BLIP_NAME(g_mission_blip);

                    char notification_text[128];
                    sprintf_s(notification_text, "A buyer wants this ~y~%s~w~. Deliver it to the garage.", g_exportable_vehicles[i].name);
                    UI::_SET_NOTIFICATION_TEXT_ENTRY("STRING");
                    UI::_ADD_TEXT_COMPONENT_STRING(notification_text);
                    UI::_DRAW_NOTIFICATION(false, true);

                    break;
                }
            }
        }
    }
    else { // --- MISSION IS ACTIVE ---
        Vehicle currentVeh = PED::IS_PED_IN_ANY_VEHICLE(player_ped, false) ? PED::GET_VEHICLE_PED_IS_IN(player_ped, false) : 0;

        if (!ENTITY::DOES_ENTITY_EXIST(currentVeh) || ENTITY::GET_ENTITY_MODEL(currentVeh) != g_exportable_vehicles[g_active_mission_index].hash) {
            fail_mission(&currentVeh);
            return;
        }

        if (ENTITY::IS_ENTITY_DEAD(currentVeh)) {
            fail_mission(&currentVeh);
            return;
        }

        Vector3 playerPos = ENTITY::GET_ENTITY_COORDS(player_ped, true);

        // THE FIX: The last parameter is 'false' to ignore height and only check 2D map distance.
        float distanceToZone = GAMEPLAY::GET_DISTANCE_BETWEEN_COORDS(playerPos.x, playerPos.y, playerPos.z, g_dropoff_location.x, g_dropoff_location.y, g_dropoff_location.z, false);

        g_can_sell_now = distanceToZone < 15.0f;

        if (g_can_sell_now) {
            ExportableVehicle& current_mission = g_exportable_vehicles[g_active_mission_index];
            float engine_health = VEHICLE::GET_VEHICLE_ENGINE_HEALTH(currentVeh);
            float body_health = VEHICLE::GET_VEHICLE_BODY_HEALTH(currentVeh);
            float health_percentage = (engine_health + body_health) / 2000.0f;
            g_potential_payout = static_cast<int>(current_mission.base_reward * health_percentage);
            if (g_potential_payout < 0) g_potential_payout = 0;

            if (IsKeyJustUp(VK_RETURN) || PadPressed(BTN_A)) {
                Money_Add(g_potential_payout);

                char notification_text[256];
                sprintf_s(notification_text, "Vehicle delivered. You earned ~g~$%d~w~.", g_potential_payout);
                RpEvents_Reward(200, notification_text);

                ENTITY::SET_ENTITY_AS_MISSION_ENTITY(currentVeh, false, true);
                VEHICLE::DELETE_VEHICLE(&currentVeh);

                if (UI::DOES_BLIP_EXIST(g_mission_blip)) {
                    UI::REMOVE_BLIP(&g_mission_blip);
                }
                g_active_mission_index = -1;
                g_mission_blip = 0;
                g_can_sell_now = false;
                g_mission_cooldown_end_time = GetTickCount64() + MISSION_COOLDOWN_MS;
            }
        }
    }
}

void CarExport_Draw() {
    if (g_active_mission_index == -1) {
        return;
    }

    float groundZ;
    if (GAMEPLAY::GET_GROUND_Z_FOR_3D_COORD(g_dropoff_location.x, g_dropoff_location.y, g_dropoff_location.z + 50.0f, &groundZ, false)) {
        GRAPHICS::DRAW_MARKER(1, g_dropoff_location.x, g_dropoff_location.y, groundZ - 0.95f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 15.0f, 15.0f, 2.0f, 255, 225, 50, 100, false, true, 2, false, NULL, NULL, false);
    }

    UI::SET_TEXT_FONT(0);
    UI::SET_TEXT_SCALE(0.0, 0.45);
    UI::SET_TEXT_COLOUR(255, 255, 255, 255);
    UI::SET_TEXT_OUTLINE();
    UI::_SET_TEXT_ENTRY("STRING");
    char mission_text[128];
    sprintf_s(mission_text, "Deliver the ~y~%s~w~ to the garage.", g_exportable_vehicles[g_active_mission_index].name);
    UI::_ADD_TEXT_COMPONENT_STRING(mission_text);
    UI::_DRAW_TEXT(0.5, 0.9);

    Vehicle currentVeh = PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), false) ? PED::GET_VEHICLE_PED_IS_IN(PLAYER::PLAYER_PED_ID(), false) : 0;
    if (ENTITY::DOES_ENTITY_EXIST(currentVeh) && ENTITY::GET_ENTITY_MODEL(currentVeh) == g_exportable_vehicles[g_active_mission_index].hash) {
        float health_percent = (VEHICLE::GET_VEHICLE_ENGINE_HEALTH(currentVeh) + VEHICLE::GET_VEHICLE_BODY_HEALTH(currentVeh)) / 20.0f;
        char health_text[64];
        sprintf_s(health_text, "Vehicle Condition: %.0f%%", health_percent);
        UI::SET_TEXT_SCALE(0.0, 0.4);
        UI::_SET_TEXT_ENTRY("STRING");
        UI::_ADD_TEXT_COMPONENT_STRING(health_text);
        UI::_DRAW_TEXT(0.5, 0.94);
    }

    if (g_can_sell_now) {
        UI::SET_TEXT_FONT(0);
        UI::SET_TEXT_SCALE(0.0, 0.45);
        UI::SET_TEXT_COLOUR(255, 255, 255, 255);
        UI::SET_TEXT_OUTLINE();
        UI::_SET_TEXT_ENTRY("STRING");
        char sellMsg[128];
        sprintf_s(sellMsg, "Press ~g~Enter~s~ or ~g~(A)~s~ to deliver vehicle for ~g~$%d~s~", g_potential_payout);
        UI::_ADD_TEXT_COMPONENT_STRING(sellMsg);
        UI::_DRAW_TEXT(0.5, 0.85);
    }
}
