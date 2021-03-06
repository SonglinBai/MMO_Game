#pragma once
#include <cstdint>

#include "bsl_net.h"
#include "olcPixelGameEngine.h"


enum class ServerStatus : uint8_t {
    IDLE,
    BUSY,
    OFFLINE,
};
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
    Game_FireBullet,
    Game_HitPlayer,
    Game_Dead
};
enum class PlayerStatus : uint32_t  {
    Alive,
    Dead,
    God
};
struct sBulletDescription {
    // The owner of the bullet
    uint32_t nOwnerID = 0;
    uint32_t nDamage = 5;

    int32_t nMass = 2;
    int32_t nBounce = 0;

    float fRadius = 0.2f;
    olc::Pixel pColor;

    // Position and velocity of the bullet
    olc::vf2d vPos;
    olc::vf2d vVel;
};
struct sPlayerDescription {
    uint32_t nUniqueID = 0;

    uint32_t nHealth = 100;
    uint32_t nEnergy = 20;
    uint32_t nMass = 10;
    uint32_t nMaxEnergy = 20;
    uint32_t nAmmo = 20;
    uint32_t nKills = 0;
    uint32_t nDeaths = 0;

    PlayerStatus status = PlayerStatus::Alive;

    // Rate of fire, num of bullet per second
    uint8_t nRof = 5;

    olc::Pixel pColor;

    float fRadius = 0.5f;

    olc::vf2d vPos;
    olc::vf2d vVel;
    olc::vf2d vAcc;
    float fSpeed = 10.0f;
};
struct sHitDescription {
    uint32_t nShooterID;
    uint32_t nSuffererID;
    uint32_t nDamage;
};
struct sDeadDescription{
    uint32_t nKillerID;
    uint32_t nSuffererID;
};