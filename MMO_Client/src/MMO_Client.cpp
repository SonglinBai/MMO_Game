#include "MMO_Common.h"

#define OLC_PGE_APPLICATION
#include "olcPixelGameEngine.h"

#define OLC_PGEX_TRANSFORMEDVIEW
#include "olcPGEX_TransformedView.h"

#include <unordered_map>

class MMOGame : public olc::PixelGameEngine, bsl::net::client_interface<GameMsg> {
public:
    MMOGame() {
        sAppName = "MMO Client";
    }

private:
    olc::TileTransformedView tv;

    std::string sWorldMap =
            "################################"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..........####...####.........#"
            "#..........#.........#.........#"
            "#..........#.........#.........#"
            "#..........#.........#.........#"
            "#..........##############......#"
            "#..............................#"
            "#..................#.#.#.#.....#"
            "#..............................#"
            "#..................#.#.#.#.....#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "#..............................#"
            "################################";

    olc::vi2d vWorldSize = {32,32};

private:
    // Map contains player information
    std::unordered_map<uint32_t, sPlayerDescription> mapObjects;
    uint32_t nPlayerID = 0;
    sPlayerDescription descPlayer;

    bool bWaitingForConnection = true;

public:
    bool OnUserCreate() override {
        tv = olc::TileTransformedView({ScreenWidth(), ScreenHeight()}, {8,8});

        if (Connect("localhost", 2696)) {
            return true;
        }
        return false;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        // Check for incoming network messages
        if (IsConnected()) {
            while (!Incoming().empty()) {
                auto msg = Incoming().pop_front().msg;
            }
        }
    }


public:
    bool OnUserCreate() override {
        return true;
    }

    bool OnUserUpdate(float fElapsedTime) override {
        return true;
    }
};

int main() {
    MMOGame demo;
    if (demo.Construct(640, 480, 2, 2))
        demo.Start();
    return 0;
}










