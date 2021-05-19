#include <iostream>
#include <unordered_map>

#include "MMO_Common.h"

class GameServer : public bsl::net::server_interface<GameMsg> {
public:
    GameServer(uint16_t nPort) : bsl::net::server_interface<GameMsg>(nPort) {}

    std::unordered_map<uint32_t, sPlayerDescription> m_mapPlayerRoster;
    // Player need to be deleted
    std::vector<uint32_t> m_vGarbageIDs;

private:
    ServerStatus getServerStatus() {
        return ServerStatus::IDLE;
    }

protected:
    bool OnClientConnect(std::shared_ptr<bsl::net::connection<GameMsg>> client) override {
        // Just allow all
        return true;
    }

    void OnClientValidated(std::shared_ptr<bsl::net::connection<GameMsg>> client) override {
        // Client passed the validation check, so send the accept message
        bsl::net::message<GameMsg> msg;
        msg.header.id = GameMsg::Client_Accept;
        client->Send(msg);
    }

    void OnClientDisconnect(std::shared_ptr<bsl::net::connection<GameMsg>> client) override {
        if (client) {
            if (m_mapPlayerRoster.find(client->GetID()) == m_mapPlayerRoster.end()) {
                // Client never added to the roster, let it disappear
            } else {
                // Client exist in the roster, remove it and add it to the garbage bin
                auto& pd = m_mapPlayerRoster[client->GetID()];
                std::cout << "[Remove]: " << pd.nUniqueID << "\n";
                m_mapPlayerRoster.erase(client->GetID());
                m_vGarbageIDs.push_back(client->GetID());
            }
        }
    }

    void OnMessage(std::shared_ptr<bsl::net::connection<GameMsg>> client, bsl::net::message<GameMsg>& msg) override {
        // Before do anything on message handle, clear the garbage first
        if (!m_vGarbageIDs.empty()) {
            for (auto pid : m_vGarbageIDs) {
                bsl::net::message<GameMsg> m;
                m.header.id = GameMsg::Game_RemovePlayer;
                m << pid;
                std::cout << "[Remove]: " << pid << "\n";
                // Send remove player message to every client
                MessageAllClients(m);
            }
            m_vGarbageIDs.clear();
        }

        // Now we can handle different message
        switch (msg.header.id) {
            // When Client send ping message, just bounce back the message
            case GameMsg::Server_GetPing: {
                MessageClient(client, msg);
                break;
            }

            // When Client send get status message, return the status of the message
            case GameMsg::Server_GetStatus: {
                bsl::net::message<GameMsg> msgStatus;
                msgStatus.header.id = GameMsg::Server_GetStatus;
                msgStatus << getServerStatus();
                MessageClient(client, msgStatus);
                break;
            }

            // When Client want to register to the server
            case GameMsg::Client_RegisterWithServer: {
                sPlayerDescription desc;
                msg >> desc;
                desc.nUniqueID = client->GetID();
                m_mapPlayerRoster.insert_or_assign(desc.nUniqueID, desc);

                // Message that return to the client the uniqueid
                bsl::net::message<GameMsg> msgSendID;
                msgSendID.header.id = GameMsg::Client_AssignID;
                msgSendID << desc.nUniqueID;
                MessageClient(client, msgSendID);

                // Send the player description to all the client
                bsl::net::message<GameMsg> msgAddPlayer;
                msgAddPlayer.header.id = GameMsg::Game_AddPlayer;
                msgAddPlayer << desc;
                MessageAllClients(msgAddPlayer);

                // Send other players' description to eh new client
                for (const auto& player : m_mapPlayerRoster) {
                    bsl::net::message<GameMsg> msgAddOtherPlayers;
                    msgAddOtherPlayers.header.id = GameMsg::Game_AddPlayer;
                    msgAddOtherPlayers << player.second;
                    MessageClient(client, msgAddOtherPlayers);
                }
                break;
            }

            // When unregister to the server
            case GameMsg::Client_UnregisterWithServer: {
                break;
            }

            // When Player updated
            case GameMsg::Game_UpdatePlayer: {
                MessageAllClients(msg, client);
                break;
            }

            // When Player Fire a bullet
            case GameMsg::Game_FireBullet: {
                MessageAllClients(msg, client);
                break;
            }

            // When Player hit someone
            // msg is shooter, sufferer, damage
            case GameMsg::Game_HitPlayer: {
                MessageAllClients(msg, client);
                break;
            }

            case GameMsg::Game_Dead: {
                MessageAllClients(msg, client);
                break;
            }
        }
    }
};

int main() {
    GameServer server(2696);
    server.Start();

    while (1) {
        server.Update(-1, true);
    }
    return 0;
}
