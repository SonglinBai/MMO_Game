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
    std::list<sBullet> listBullets;

    bool bWaitingForConnection = true;

    bool bFollowObject = true;

    // Get ping 1 per second
    float fPingTime = 0.0f;
    float fPing = 0.0f;

    // Get status 5 per second
    float fStatusTime = 0.0f;
    ServerStatus serverStatus = ServerStatus::IDLE;

    // Time for ROF(rate of fire)
    float fROFTime = 0.0f;

private:
    void setMap(std::string path) {
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

    inline void Shoot(ShootDirection direction) {
        if (fROFTime<1.0f/mapObjects[nPlayerID].nRof) return;
        fROFTime = 0;
        olc::vf2d v = mapObjects[nPlayerID].vVel;
        switch (direction) {
            case ShootDirection::UP: {
                v += {0.0f, -1.0f * 2.0f};
                break;
            }
            case ShootDirection::DOWN: {
                v += {0.0f, 1.0f * 2.0f};
                break;
            }
            case ShootDirection::LEFT: {
                v += {-1.0f * 2.0f, 0.0f};
                break;
            }
            case ShootDirection::RIGHT: {
                v += {1.0f * 2.0f, 0.0f};
                break;
            }
        }
        // Declare a bullet
        sBullet bullet = {nPlayerID, 1, 2, 0.1f,
                          olc::Pixel(255, 0, 0),
                          mapObjects[nPlayerID].vPos,
                          v};
        listBullets.push_back(bullet);
    }


public:
    bool OnUserCreate() override {
        tv = olc::TileTransformedView({ScreenWidth(), ScreenHeight()}, {32, 32});
        setMap("resources/map/map_demo.txt");
        // Connect to the server
        if (Connect("localhost", 2696)) {
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
                }
            }
        }
        // When connecting to server, display blank blue
        if (bWaitingForConnection) {
            Clear(olc::DARK_BLUE);
            DrawString({10, 10}, "Waiting To connect...", olc::WHITE);
            return true;
        }

        // Control of Player Object, initialize playesr to left conor of the world
        mapObjects[nPlayerID].vVel = {0.0f, 0.0f};
        if (GetKey(olc::Key::W).bHeld) mapObjects[nPlayerID].vVel += {0.0f, -1.0f};
        if (GetKey(olc::Key::S).bHeld) mapObjects[nPlayerID].vVel += {0.0f, +1.0f};
        if (GetKey(olc::Key::A).bHeld) mapObjects[nPlayerID].vVel += {-1.0f, 0.0f};
        if (GetKey(olc::Key::D).bHeld) mapObjects[nPlayerID].vVel += {+1.0f, 0.0f};

        if (mapObjects[nPlayerID].vVel.mag2() > 0)
            mapObjects[nPlayerID].vVel = mapObjects[nPlayerID].vVel.norm() * 6.0f;

        // Use arrow key to control player to shoot
        if (GetKey(olc::Key::UP).bHeld) Shoot(ShootDirection::UP);
        if (GetKey(olc::Key::DOWN).bHeld) Shoot(ShootDirection::DOWN);
        if (GetKey(olc::Key::LEFT).bHeld) Shoot(ShootDirection::LEFT);
        if (GetKey(olc::Key::RIGHT).bHeld) Shoot(ShootDirection::RIGHT);


        // Press Space key to toggle Follow mode
        if (GetKey(olc::Key::SPACE).bReleased) bFollowObject = !bFollowObject;


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
            olc::vi2d vAreaBR = (vCurrentCell.min(vTargetCell) + olc::vi2d(1, 1)).min(vWorldSize);

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
            olc::vi2d vAreaTL = (vCurrentCell.min(vTargetCell) - olc::vi2d(1, 1).max({0, 0}));
            olc::vi2d vAreaBR = (vCurrentCell.min(vTargetCell) + olc::vi2d(1, 1).min(vWorldSize));
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
                        float fOverlap = bullet.fRadius - vRayToNearest.mag();
                        if (std::isnan(fOverlap)) fOverlap = 0;

                        if (fOverlap > 0) {
                            // If happen collision, nBounce minus 1, and reverse the velocity
                            bullet.nBounce--;
                            bullet.vVel *= -vRayToNearest.norm();
                        }
                    }
                }
            }

            // Set the object new position
            bullet.vPos = vPotentialPosition;
        }
        // Remove all the bullet which can't bounce
        listBullets.remove_if([](sBullet b) { return b.nBounce < 0; });


        // Clear World
        Clear(olc::BLACK);

        // Handle Pan & Zoom
        if (GetMouse(2).bPressed) tv.StartPan(GetMousePos());
        if (GetMouse(2).bHeld) tv.UpdatePan(GetMousePos());
        if (GetMouse(2).bReleased) tv.EndPan(GetMousePos());
        if (GetMouseWheel() > 0) tv.ZoomAtScreenPos(1.5f, GetMousePos());
        if (GetMouseWheel() < 0) tv.ZoomAtScreenPos(0.75f, GetMousePos());

        // Display Server status
        DrawString({10, 10}, "Server: " + (std::string) magic_enum::enum_name(serverStatus));

        // Display Ping
        DrawString({10, 20}, "Ping: " + std::to_string(fPing) + "ms");

        // Check follow mode or not
        if (bFollowObject) {
            // Set offest to make object in middle of the screen
            tv.SetWorldOffset(mapObjects[nPlayerID].vPos -
                              tv.ScaleToWorld(olc::vf2d(ScreenWidth() / 2.0f, ScreenHeight() / 2.0f)));
            DrawString({10, 30}, "Following Object");
        }


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
            olc::vi2d vNameSize = GetTextSizeProp("ID: " + std::to_string(object.first));
            tv.DrawStringPropDecal(
                    object.second.vPos - olc::vf2d{vNameSize.x * 0.5f * 0.25f * 0.125f, -object.second.fRadius * 1.25f},
                    "ID: " + std::to_string(object.first), olc::BLUE, {1, 1});
        }
        // Draw Bullet
        for (auto &bullet : listBullets) {
            tv.FillCircle(bullet.vPos, bullet.fRadius, bullet.pColor);
        }

        // Send player description
        bsl::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Game_UpdatePlayer;
        msg << mapObjects[nPlayerID];
        Send(msg);
        return true;
    }
};

int main() {
    MMOGame demo;
    if (demo.Construct(800, 600, 1, 1))
        demo.Start();
    return 0;
}










