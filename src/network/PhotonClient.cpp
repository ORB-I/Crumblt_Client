#include "PhotonClient.h"
#include <cstdio>

// TODO: Replace stub with real Photon Realtime SDK calls.
// Photon SDK: https://www.photonengine.com/sdks#realtime-cpp
// Room name convention: "crumblt-game-<gameId>"

void PhotonClient::connect(const std::string& gameId) {
    m_roomId = "crumblt-game-" + gameId;
    // TODO: LoadBalancingClient::connect(), then opJoinOrCreateRoom(m_roomId)
    m_connected   = true;
    m_playerCount = 1;
    printf("[Photon] Stub: would join room '%s'\n", m_roomId.c_str());
}

void PhotonClient::disconnect() {
    if (!m_connected) return;
    // TODO: LoadBalancingClient::disconnect()
    m_connected = false;
    printf("[Photon] Stub: disconnected\n");
}

void PhotonClient::update() {
    if (!m_connected) return;
    // TODO: LoadBalancingClient::service() — must be called every frame
}

void PhotonClient::sendNodeUpdate(const std::string& nodeId, float x, float y, float z) {
    if (!m_connected) return;
    // TODO: raiseEvent with nodeId + position, reliable or unreliable depending on frequency
    (void)nodeId; (void)x; (void)y; (void)z;
}
