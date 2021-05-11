#pragma once
#include <cstdint>

#include "bsl_net.h"
#include "olcPixelGameEngine.h"

enum class GameMsg : uint32_t {
    Server_GetStatus,
    Server_GetPing,

    Client_Accept,
    Client_AssignID,
    Client_RegisterWithServer,
    Client_UnregisterWithServer,

    Game_AddPlayer,
    Game_RemovePlayer,
    Game_UpdatePlayer,

    Bullet_Fire,
    Bullet_Hit,
};

struct sPlayerDescription {
    uint32_t nUniqueID = 0;
    uint32_t nAvatarID = 0;

    uint32_t nHealth = 100;
    uint32_t nAmmo = 20;
    uint32_t nKills = 0;
    uint32_t nDeaths = 0;

    // Rate of fire, num of bullet per second
    uint8_t nRof = 2;

    olc::Pixel pColor;

    float fRadius = 0.5f;

    olc::vf2d vPos;
    olc::vf2d vVel;
};

enum class ServerStatus : uint8_t {
    IDLE,
    BUSY,
    OFFLINE,
};


struct sBullet {
    // The owner of the bullet
    uint32_t nOwnerID = 0;
    uint32_t nDamage = 0;

    int32_t nBounce = 0;

    float fRadius = 0.1f;
    olc::Pixel pColor;

    // Position and velocity of the bullet
    olc::vf2d vPos;
    olc::vf2d vVel;
};