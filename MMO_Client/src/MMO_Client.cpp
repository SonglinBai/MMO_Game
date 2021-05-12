#define OLC_IMAGE_STB
#define OLC_PGE_APPLICATION

#include "olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW

#include "olcPGEX_TransformedView.h"

// Must include after game engine
#include "MMO_Common.h"

#include <unordered_map>
#include <fstream>
#include "magic_enum.hpp"

enum class ShootDirection : uint8_t {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class MMOGame : public olc::PixelGameEngine, bsl::net::client_interface<GameMsg> {
public:
    MMOGame() {
        sAppName = "MMO Client";
    }

private:
    olc::TileTransformedView tv;

    std::string sWorldMap;

    olc::vi2d vWorldSize = {0, 0};

private:
    // Map contains player information
    std::unordered_map<uint32_t, sPlayerDescription> mapObjects;
    uint32_t nPlayerID = 0;
    sPlayerDescription descPlayer;

    // List contain all the bullets
    std::list<sBulletDescription> listBullets;

    bool bWaitingForConnection = true;

    bool bFollowObject = true;

    // Get ping per second
    float fPingTime = 0.0f;
    float fPing = 0.0f;

    // Get status 5 per second
    float fStatusTime = 0.0f;
    ServerStatus serverStatus = ServerStatus::IDLE;

    // Time for ROF(rate of fire)
    float fROFTime = 0.0f;

    // Time for increase energy
    float fEnergyTime = 0.0f;

    float fSpawnTime = 5.0f;

private:
    // Handle Network
    void HandleNetwork() {
        // Check for incoming network messages
        if (IsConnected()) {
            while (!Incoming().empty()) {
                auto msg = Incoming().pop_front().msg;

                switch (msg.header.id) {
                    // When Server back the status message
                    case (GameMsg::Server_GetStatus): {
                        fStatusTime = 0;
                        msg >> serverStatus;
                        break;
                    }

                        // When Server bounce back ping message
                    case (GameMsg::Server_GetPing): {
                        fPingTime = 0;
                        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
                        std::chrono::system_clock::time_point timeThen;
                        msg >> timeThen;
                        fPing = std::chrono::duration<float>(timeNow - timeThen).count() * 1000;
                        break;
                    }

                        // When accept the client, send the player description to the server to register on server
                    case (GameMsg::Client_Accept): {
                        std::cout << "Server accepted client\n";
                        bsl::net::message<GameMsg> msg;
                        msg.header.id = GameMsg::Client_RegisterWithServer;
                        descPlayer.vPos = {3.0f, 3.0f};
                        msg << descPlayer;
                        Send(msg);
                        break;
                    }
                        // When server assign our id
                    case (GameMsg::Client_AssignID): {
                        msg >> nPlayerID;
                        std::cout << "Assigned Client ID = " << nPlayerID << "\n";
                        break;
                    }
                        // When Server add player
                    case (GameMsg::Game_AddPlayer): {
                        sPlayerDescription desc;
                        msg >> desc;
                        mapObjects.insert_or_assign(desc.nUniqueID, desc);

                        if (desc.nUniqueID == nPlayerID) {
                            // Successfully add our own player to the game world
                            bWaitingForConnection = false;
                        }
                        break;
                    }
                        // When Server remove player
                    case (GameMsg::Game_RemovePlayer): {
                        uint32_t nRemovalID = 0;
                        msg >> nRemovalID;
                        mapObjects.erase(nRemovalID);
                        break;
                    }
                        // When Server update player information
                    case (GameMsg::Game_UpdatePlayer): {
                        sPlayerDescription desc;
                        msg >> desc;
                        mapObjects.insert_or_assign(desc.nUniqueID, desc);
                        break;
                    }

                    case (GameMsg::Game_FireBullet): {
                        sBulletDescription bullet;
                        msg >> bullet;
                        listBullets.push_back(bullet);
                        break;
                    }

                    case (GameMsg::Game_HitPlayer): {
                        sHitDescription desc;
                        msg >> desc;
                        if (desc.nSuffererID == nPlayerID) {
                            if (desc.nDamage >= mapObjects[nPlayerID].nHealth) {
                                mapObjects[nPlayerID].nHealth = 0;
                                mapObjects[nPlayerID].status = PlayerStatus::Dead;
                                mapObjects[nPlayerID].vPos = {-3.0f, -3.0f};
                                mapObjects[nPlayerID].nDeaths++;
                                sDeadDescription d = {desc.nShooterID, desc.nSuffererID};
                                bsl::net::message<GameMsg> m;
                                m.header.id = GameMsg::Game_Dead;
                                m << d;
                                Send(m);
                            } else mapObjects[nPlayerID].nHealth -= desc.nDamage;
                        }
                        break;
                    }

                    case (GameMsg::Game_Dead): {
                        sDeadDescription desc;
                        msg >> desc;
                        if (desc.nKillerID == nPlayerID) {
                            mapObjects[nPlayerID].nKills++;
                        }
                        break;
                    }
                }
            }
        }
        // When connecting to server, display blank blue
    }

    void DisplayHUD() {

        // Display Server status
        DrawString({10, 10}, "Server: " + (std::string) magic_enum::enum_name(serverStatus));

        // Display Ping
        DrawString({10, 20}, "Ping: " + std::to_string(fPing) + "ms");

        if (bFollowObject) {
            DrawString({10, 30}, "Following Object");
        }

        if (mapObjects[nPlayerID].status == PlayerStatus::Dead) {
            DrawString({10, 40}, "Spawn:" + std::to_string(fSpawnTime), olc::WHITE, 2);
        }


        // Display energy and health
        std::string sHealth = "Health:" + std::to_string(mapObjects[nPlayerID].nHealth);
        std::string sEnergy = "Energy:" + std::to_string(mapObjects[nPlayerID].nEnergy) + "/" +
                              std::to_string(mapObjects[nPlayerID].nMaxEnergy);

        DrawString({10, GetWindowSize().y - 80}, "pos:(" + std::to_string(mapObjects[nPlayerID].vPos.x) + "," +
                                                 std::to_string(mapObjects[nPlayerID].vPos.y) + ")");
        DrawString({10, GetWindowSize().y - 70}, "acceleration:" + std::to_string(mapObjects[nPlayerID].vAcc.mag()));
        DrawString({10, GetWindowSize().y - 60}, "velocity:" + std::to_string(mapObjects[nPlayerID].vVel.mag()));

        DrawString({10, GetWindowSize().y - 40}, sHealth, olc::RED, 2);
        DrawString({10, GetWindowSize().y - 20}, sEnergy, olc::GREEN, 2);

        std::string sKill = "Kill:" + std::to_string(mapObjects[nPlayerID].nKills);
        std::string sDeath = "Death:" + std::to_string(mapObjects[nPlayerID].nDeaths);
        DrawString({(GetWindowSize().x - 70), 10}, sKill, olc::GREEN, 1.25f);
        DrawString({(GetWindowSize().x - 70), 25}, sDeath, olc::YELLOW, 1.25f);

    }

    void HandleInput(float fElapsedTime) {
        if (GetKey(olc::Key::Q).bHeld && mapObjects[nPlayerID].nEnergy > 0 && mapObjects[nPlayerID].vVel.mag2() > 0) {
            mapObjects[nPlayerID].fSpeed = 15.0f;
            if (fEnergyTime > 0.1f) {
                mapObjects[nPlayerID].nEnergy--;
                fEnergyTime = 0;
            }
            if (mapObjects[nPlayerID].nEnergy < 0) mapObjects[nPlayerID].nEnergy = 0;
        } else {
            mapObjects[nPlayerID].fSpeed = 8.0f;
            if (fEnergyTime > 0.3f) {
                mapObjects[nPlayerID].nEnergy++;
                fEnergyTime = 0;
            }
            if (mapObjects[nPlayerID].nEnergy > mapObjects[nPlayerID].nMaxEnergy)
                mapObjects[nPlayerID].nEnergy = mapObjects[nPlayerID].nMaxEnergy;
        }

        // Get Control Acc
        olc::vf2d vControlAcc = {0.0f, 0.0f};
        olc::vf2d vEnvAcc = {0.0f, 0.0f};
        if (GetKey(olc::Key::W).bHeld) vControlAcc += {0.0f, -1.0f};
        if (GetKey(olc::Key::S).bHeld) vControlAcc += {0.0f, +1.0f};
        if (GetKey(olc::Key::A).bHeld) vControlAcc += {-1.0f, 0.0f};
        if (GetKey(olc::Key::D).bHeld) vControlAcc += {+1.0f, 0.0f};
        if (vControlAcc.mag2() > 0)
            vControlAcc = vControlAcc.norm() * 80.0f;
        if (mapObjects[nPlayerID].vVel.mag2() > 0)
            vEnvAcc = -mapObjects[nPlayerID].vVel.norm() * 50.0f;

        mapObjects[nPlayerID].vAcc = vEnvAcc + vControlAcc;

        olc::vf2d before = mapObjects[nPlayerID].vVel.norm();
        if (mapObjects[nPlayerID].vAcc.mag2() > 0)
            mapObjects[nPlayerID].vVel += mapObjects[nPlayerID].vAcc * fElapsedTime;
        if (mapObjects[nPlayerID].vVel.norm().dot(before) < 0)
            mapObjects[nPlayerID].vVel = {0.0f, 0.0f};
        if (mapObjects[nPlayerID].vVel.mag() > mapObjects[nPlayerID].fSpeed)
            mapObjects[nPlayerID].vVel = mapObjects[nPlayerID].fSpeed * mapObjects[nPlayerID].vVel.norm();

        // Use arrow key to control player to shoot
        if (GetKey(olc::Key::UP).bHeld) Shoot({0.0f, -1.0f});
        if (GetKey(olc::Key::DOWN).bHeld) Shoot({0.0f, 1.0f});
        if (GetKey(olc::Key::LEFT).bHeld) Shoot({-1.0f, 0.0f});
        if (GetKey(olc::Key::RIGHT).bHeld) Shoot({1.0f, 0.0f});


        // Press Space key to toggle Follow mode
        if (GetKey(olc::Key::SPACE).bReleased) bFollowObject = !bFollowObject;

        // Handle Pan & Zoom
        if (GetMouse(2).bPressed) tv.StartPan(GetMousePos());
        if (GetMouse(2).bHeld) tv.UpdatePan(GetMousePos());
        if (GetMouse(2).bReleased) tv.EndPan(GetMousePos());
        if (GetMouseWheel() > 0) tv.ZoomAtScreenPos(1.5f, GetMousePos());
        if (GetMouseWheel() < 0) tv.ZoomAtScreenPos(0.75f, GetMousePos());

        // Check follow mode or not
        if (bFollowObject) {
            // Set offest to make object in middle of the screen
            tv.SetWorldOffset(mapObjects[nPlayerID].vPos -
                              tv.ScaleToWorld(olc::vf2d(ScreenWidth() / 2.0f, ScreenHeight() / 2.0f)));
        }
    }

    void SetMap(std::string path) {
        std::ifstream file(path);

        std::string line;
        while (std::getline(file, line)) {
            sWorldMap.append(line);
            vWorldSize.y++;
        }
        vWorldSize.x = line.length();
        std::cout << "Map " << path << " loaded\nSize: (" << vWorldSize.x << "," << vWorldSize.y << ")\n";
    }

    void GetPing() {
        bsl::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Server_GetPing;

        std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();

        msg << timeNow;
        Send(msg);
    }

    void GetStatus() {
        bsl::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Server_GetStatus;
        Send(msg);
    }

    inline void Shoot(olc::vf2d direction) {
        if (fROFTime < 1.0f / mapObjects[nPlayerID].nRof || mapObjects[nPlayerID].status == PlayerStatus::Dead) return;
        fROFTime = 0;
        olc::vf2d v;
//        if (mapObjects[nPlayerID].vVel.mag2() > 0) {
//            v = mapObjects[nPlayerID].vVel.norm();
//            if (direction.norm() == -v) v = direction;
//            else v += direction;
//        } else v = direction;
        v = direction.norm() * 20.0f + mapObjects[nPlayerID].vVel;

        // Declare a bullet
        sBulletDescription bullet = {nPlayerID, 5, 2, 1, 0.2f,
                                     olc::Pixel(255, 0, 0),
                                     mapObjects[nPlayerID].vPos,
                                     v};
        listBullets.push_back(bullet);
        // Send bullet fire message
        bsl::net::message<GameMsg> FireMsg;
        FireMsg.header.id = GameMsg::Game_FireBullet;
        FireMsg << bullet;
        Send(FireMsg);
    }

    inline olc::vf2d reflect(olc::vf2d d, olc::vf2d n) {
        olc::vf2d r;
        r = d.norm() - 2 * (d.norm().dot(n.norm())) * n.norm();
        r *= d.mag();
        return r;
    }

    inline void HitPlayer(uint32_t shooterID, uint32_t suffererID, uint32_t damage) {
        bsl::net::message<GameMsg> HitMsg;
        HitMsg.header.id = GameMsg::Game_HitPlayer;
        sHitDescription desc = {shooterID, suffererID, damage};
        HitMsg << desc;
        Send(HitMsg);
    }

public:
    bool OnUserCreate() override {
        tv = olc::TileTransformedView({ScreenWidth(), ScreenHeight()}, {32, 32});
        SetMap("resources/map/map_demo.txt");
        // Connect to the server
        if (Connect("127.0.0.1", 2696)) {
            return true;
        }
        return false;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Ping Server every second
        fPingTime += fElapsedTime;
        if (fPingTime > 1.0f) {
            GetPing();
        }
        fStatusTime += fElapsedTime;
        if (fStatusTime > 5.0f) {
            GetStatus();
        }
        fROFTime += fElapsedTime;
        fEnergyTime += fElapsedTime;

        // Handle network message
        HandleNetwork();

        if (bWaitingForConnection) {
            Clear(olc::DARK_BLUE);
            DrawString({10, 10}, "Waiting To connect...", olc::WHITE);
            return true;
        }

        if (mapObjects[nPlayerID].status == PlayerStatus::Dead) {
            if (fSpawnTime <= 0) {
                mapObjects[nPlayerID].status = PlayerStatus::Alive;
                mapObjects[nPlayerID].vPos = {3.0f, 3.0f};
                mapObjects[nPlayerID].nHealth = 100;
                mapObjects[nPlayerID].nEnergy = 100;
                fSpawnTime = 5.0f;
            } else {
                fSpawnTime -= fElapsedTime;
            }
        }

        // Handle User input
        HandleInput(fElapsedTime);

        // update objects locally
        for (auto &object : mapObjects) {
            // Caculate the new positon of the player
            // Because the frame rate is different, so we need to use elapsed time to get approximate speed
            olc::vf2d vPotentialPosition = object.second.vPos + object.second.vVel * fElapsedTime;

            // Get the region of world cells that may have collision
            olc::vi2d vCurrentCell = object.second.vPos.floor();
            olc::vi2d vTargetCell = vPotentialPosition;
            // TopLeft
            olc::vi2d vAreaTL = (vCurrentCell.min(vTargetCell) - olc::vi2d(1, 1)).max({0, 0});
            // BottomRight
            olc::vi2d vAreaBR = (vCurrentCell.max(vTargetCell) + olc::vi2d(1, 1)).min(vWorldSize);

            //TODO: Draw collision area, delete later
            tv.FillRectDecal(vAreaTL, vAreaBR - vAreaTL + olc::vi2d(1, 1), olc::Pixel(0, 255, 255, 32));

            olc::vf2d vRayToNearest;

            // Iterate through each cell in collision test area
            olc::vi2d vCell;
            for (vCell.y = vAreaTL.y; vCell.y <= vAreaBR.y; vCell.y++) {
                for (vCell.x = vAreaTL.x; vCell.x <= vAreaBR.x; vCell.x++) {
                    // Check the cell is solid or not
                    if (sWorldMap[vCell.y * vWorldSize.x + vCell.x] == '#') {
                        olc::vf2d vNearestPoint;
                        vNearestPoint.x = std::max(float(vCell.x), std::min(vPotentialPosition.x, float(vCell.x + 1)));
                        vNearestPoint.y = std::max(float(vCell.y), std::min(vPotentialPosition.y, float(vCell.y + 1)));

                        vRayToNearest = vNearestPoint - vPotentialPosition;
                        float fOverlap = object.second.fRadius - vRayToNearest.mag();
                        if (std::isnan(fOverlap)) fOverlap = 0;

                        if (fOverlap > 0) {
                            vPotentialPosition = vPotentialPosition - vRayToNearest.norm() * fOverlap;
                        }
                    }
                }
            }

            // Check collision with other object
            for (auto &targetObject : mapObjects) {
                // Ignore your self
                if (object.first == targetObject.first) continue;

                float fDistance = (object.second.vPos - targetObject.second.vPos).mag();
                // Collision happened
                if (fDistance <= object.second.fRadius + targetObject.second.fRadius) {
                    // Move position
                    float fOverlap = 0.5f * (fDistance - object.second.fRadius - object.second.fRadius);
                    object.second.vPos -= fOverlap / fDistance * (object.second.vPos - targetObject.second.vPos);
                    targetObject.second.vPos += fOverlap / fDistance * (object.second.vPos - targetObject.second.vPos);
                    // Caculate velocity
//                    olc::vf2d n = (targetObject.second.vPos - object.second.vPos).norm();
//                    olc::vf2d k = object.second.vVel - targetObject.second.vVel;
//                    float p = 20.0f * (n.x * k.x + n.y * k.y) / (object.second.nMass + targetObject.second.nMass);
//                    object.second.vAcc = object.second.vAcc- p * targetObject.second.nMass * olc::vf2d(n.x, n.y);
//                    targetObject.second.vAcc =
//                            targetObject.second.vAcc+ p * object.second.nMass * olc::vf2d(n.x, n.y);
                }

            }

            // Set the object new position
            object.second.vPos = vPotentialPosition;
        }

        // Update bullets locally
        // TODO: Bullets collision test has some problem, need to fix
        for (auto &bullet : listBullets) {
            // Caculate the new position of the bullet
            olc::vf2d vPotentialPosition = bullet.vPos + bullet.vVel * fElapsedTime;

            // Get the region of world cells that may have collision
            olc::vi2d vCurrentCell = bullet.vPos.floor();
            olc::vi2d vTargetCell = vPotentialPosition;
            olc::vi2d vAreaTL = (vCurrentCell.min(vTargetCell) - olc::vi2d(1, 1)).max({0, 0});
            olc::vi2d vAreaBR = (vCurrentCell.max(vTargetCell) + olc::vi2d(1, 1)).min(vWorldSize);
            //TODO: Draw collision area, delete later
            tv.FillRectDecal(vAreaTL, vAreaBR - vAreaTL + olc::vi2d(1, 1), olc::Pixel(0, 255, 255, 32));
            olc::vf2d vRayToNearest;

            // Iterate through each cell in collision test area
            olc::vi2d vCell;
            bool bCollisionHappen = false;
            for (vCell.y = vAreaTL.y; vCell.y <= vAreaBR.y; vCell.y++) {
                for (vCell.x = vAreaTL.x; vCell.x <= vAreaBR.x; vCell.x++) {
                    // Check the cell is solid or not
                    if (sWorldMap[vCell.y * vWorldSize.x + vCell.x] == '#') {
                        olc::vf2d vNearestPoint;
                        vNearestPoint.x = std::max(float(vCell.x), std::min(vPotentialPosition.x, float(vCell.x + 1)));
                        vNearestPoint.y = std::max(float(vCell.y), std::min(vPotentialPosition.y, float(vCell.y + 1)));

                        vRayToNearest = vNearestPoint - vPotentialPosition;
                        float fOverlap = bullet.fRadius - vRayToNearest.mag();
                        if (std::isnan(fOverlap)) fOverlap = 0;

                        if (fOverlap > 0) {
                            bCollisionHappen = true;
                            vPotentialPosition = vPotentialPosition - vRayToNearest.norm() * fOverlap;
                            // If happen collision, nBounce minus 1, and reverse the velocity
                            if (bullet.nBounce > 0)
                                bullet.vVel = reflect(bullet.vVel, -vRayToNearest);
                            bullet.nBounce--;
                            break;
                        }
                    }
                }
                if (bCollisionHappen) break;
            }

            for (auto &targetObject : mapObjects) {
                // Ignore your self
                if (bullet.nOwnerID == targetObject.first) continue;

                float fDistance = (bullet.vPos - targetObject.second.vPos).mag();
                // Collision happened
                if (fDistance <= bullet.fRadius + targetObject.second.fRadius) {
                    HitPlayer(bullet.nOwnerID, targetObject.first, bullet.nDamage);
                    bullet.nBounce = -1;
                }
            }

            // Set the object new position
            bullet.vPos = vPotentialPosition;
        }
        // Remove all the bullet which can't bounce
        listBullets.remove_if([](sBulletDescription b) { return b.nBounce < 0; });


        // Clear World
        Clear(olc::BLACK);

        // Draw World
        olc::vi2d vTL = tv.GetTopLeftTile().max({0, 0});
        olc::vi2d vBR = tv.GetBottomRightTile().min(vWorldSize);
        olc::vi2d vTile;
        for (vTile.y = vTL.y; vTile.y < vBR.y; vTile.y++) {
            for (vTile.x = vTL.x; vTile.x < vBR.x; vTile.x++) {
                if (sWorldMap[vTile.y * vWorldSize.x + vTile.x] == '#') {
                    tv.DrawRect(vTile, {1.0f, 1.0f});
                    tv.DrawRect(olc::vf2d(vTile) + olc::vf2d(0.1f, 0.1f), {0.8f, 0.8f});
                }
            }
        }

        // Draw World Objects
        for (auto &object : mapObjects) {
            // Draw Boundary
            tv.DrawCircle(object.second.vPos, object.second.fRadius);

            // Draw Velocity
            if (object.second.vVel.mag2() > 0)
                tv.DrawLine(object.second.vPos, object.second.vPos + object.second.vVel.norm() * object.second.fRadius,
                            olc::MAGENTA);

            // Draw Name
            std::string sName = "ID:" + std::to_string(object.first);
            tv.DrawStringPropDecal(
                    object.second.vPos -
                    olc::vf2d{GetTextSizeProp(sName).x * 0.5f * 0.25f * 0.125f, +object.second.fRadius * 1.75f},
                    sName, olc::BLUE, {1, 1});
            // Draw health and energy
            std::string sHealth = "HP:" + std::to_string(object.second.nHealth);
            tv.DrawStringPropDecal(
                    object.second.vPos -
                    olc::vf2d{GetTextSizeProp(sHealth).x * 0.5f * 0.25f * 0.125f, -object.second.fRadius * 1.25f},
                    sHealth, olc::RED, {1, 1});
        }
        // Draw Bullet
        for (auto &bullet : listBullets) {
            tv.FillCircle(bullet.vPos, bullet.fRadius, bullet.pColor);
        }

        // Display HUD
        DisplayHUD();

        // Send player description
        bsl::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Game_UpdatePlayer;
        msg << mapObjects[nPlayerID];
        Send(msg);
        return true;
    }

    bool OnUserDestroy() override {

    }
};

int main() {
    MMOGame demo;
    if (demo.Construct(800, 600, 1, 1))
        demo.Start();
    return 0;
}










