#define OLC_IMAGE_STB
#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

class CircleVSRect : public olc::PixelGameEngine {
public:
    CircleVSRect() { sAppName = "Circle Vs Rectangle"; }

private:
    // Use extension to enable drag and scale the view
    olc::TileTransformedView tv;

    struct sWorldObject {
        olc::vf2d vPos;
        olc::vf2d vVel;
        float fRadius = 0.5f;
    };
    sWorldObject object;

    // Use string to set the world map
    // '#' means block, '.' means space player can move
    std::string sWorldMap;
    olc::vi2d vWorldSize = {0, 0};

    // Flag means follow player or not
    bool bFollowObject = false;

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


public:
    bool OnUserCreate() override {
        // Create "Tiled World", each tile is 32 * 32 screen pixels
        // The coordinates for drawing is set to the tile now
        setMap("resources/map/map_demo.txt");
        tv = olc::TileTransformedView({ScreenWidth(), ScreenHeight()}, {32, 32});
        object.vPos = {3.0f, 3.0f};
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Control of Player Object
        // Initialized the velocity of the object
        object.vVel = {0.0f, 0.0f};

        // Use WASD to control player, add a 2d vector to the velocity
        if (GetKey(olc::Key::W).bHeld) object.vVel += {0.0f, -1.0f};
        if (GetKey(olc::Key::S).bHeld) object.vVel += {0.0f, +1.0f};
        if (GetKey(olc::Key::A).bHeld) object.vVel += {-1.0f, 0.0f};
        if (GetKey(olc::Key::D).bHeld) object.vVel += {+1.0f, 0.0f};

        // Set the speed of the player, when hold the shift key, player can move fast
        if (object.vVel.mag2() > 0)
            object.vVel = object.vVel.norm() * (GetKey(olc::Key::SHIFT).bHeld ? 20.0f : 10.0f);

        // Press Space key to toggle Follow mode
        if (GetKey(olc::Key::SPACE).bReleased) bFollowObject = !bFollowObject;

        // Caculate the new positon of the player
        // Because the frame rate is different, so we need to use elapsed time to get approximate speed
        olc::vf2d vPotentialPosition = object.vPos + object.vVel * fElapsedTime;

        // Get the region of world cells that may have collision
        olc::vi2d vCurrentCell = object.vPos.floor();
        olc::vi2d vTargetCell = vPotentialPosition;
        // TopLeft
        olc::vi2d vAreaTL = (vCurrentCell.min(vTargetCell) - olc::vi2d(1, 1)).max({0, 0});
        // BottomRight
        olc::vi2d vAreaBR = (vCurrentCell.min(vTargetCell) + olc::vi2d(1, 1)).min(vWorldSize);

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
                    float fOverlap = object.fRadius - vRayToNearest.mag();
                    if (std::isnan(fOverlap)) fOverlap = 0;

                    if (fOverlap > 0) {
                        vPotentialPosition = vPotentialPosition - vRayToNearest.norm() * fOverlap;
                    }
                }
            }
        }

        // Set the object new position
        object.vPos = vPotentialPosition;

        // Clear World, do it before draw any component
        Clear(olc::VERY_DARK_BLUE);

        // Check follow mode or not
        if (bFollowObject) {
            // Set offest to make object in middle of the screen
            tv.SetWorldOffset(object.vPos - tv.ScaleToWorld(olc::vf2d(ScreenWidth() / 2.0f, ScreenHeight() / 2.0f)));
            DrawString({10, 10}, "Following Object");
        }

        // Handle Pan & Zoom
        if (GetMouse(2).bPressed) tv.StartPan(GetMousePos());
        if (GetMouse(2).bHeld) tv.UpdatePan(GetMousePos());
        if (GetMouse(2).bReleased) tv.EndPan(GetMousePos());
        if (GetMouseWheel() > 0) tv.ZoomAtScreenPos(2.0f, GetMousePos());
        if (GetMouseWheel() < 0) tv.ZoomAtScreenPos(0.5f, GetMousePos());

        // Draw World
        olc::vi2d vTL = tv.GetTopLeftTile().max({0, 0});
        olc::vi2d vBR = tv.GetBottomRightTile().min(vWorldSize);
        olc::vi2d vTile;
        for (vTile.y = vTL.y; vTile.y < vBR.y; vTile.y++) {
            for (vTile.x = vTL.x; vTile.x < vBR.x; vTile.x++) {
                if (sWorldMap[vTile.y * vWorldSize.x + vTile.x] == '#') {
                    tv.DrawRect(vTile, {1.0f,1.0f}, olc::WHITE);
                    tv.DrawLine(vTile, vTile + olc::vf2d(1.0f, 1.0f), olc::WHITE);
                    tv.DrawLine(vTile + olc::vf2d(0.0f, 1.0f), vTile + olc::vf2d(1.0f, 0.0f), olc::WHITE);
                }
            }
        }

        // Draw the collision test area
        tv.FillRectDecal(vAreaTL, vAreaBR - vAreaTL + olc::vi2d(1,1), olc::Pixel(0,255,255,32));

        // Draw object
        tv.DrawCircle(object.vPos, object.fRadius, olc::WHITE);

        // Draw velocity
        if (object.vVel.mag2() > 0) {
            tv.DrawLine(object.vPos, object.vPos + object.vVel.norm() * object.fRadius, olc::MAGENTA);
        }

        return true;
    }
};

int main() {
    CircleVSRect demo;
    if (demo.Construct(640, 480, 2, 2))
        demo.Start();
    return 0;
}
























