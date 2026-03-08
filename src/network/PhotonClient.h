#pragma once
#include <string>
#include <functional>
#include "../scene/Scene.h"

// Callback types
using OnPlayerJoin  = std::function<void(const std::string& playerId)>;
using OnPlayerLeave = std::function<void(const std::string& playerId)>;
using OnNodeUpdate  = std::function<void(const std::string& nodeId, float x, float y, float z)>;

class PhotonClient {
public:
    PhotonClient() = default;
    ~PhotonClient() { disconnect(); }

    // Connect to / create a Photon room for this gameId
    void connect(const std::string& gameId);
    void disconnect();
    void update(); // Call every frame - dispatches callbacks

    // Send a node position update to all peers
    void sendNodeUpdate(const std::string& nodeId, float x, float y, float z);

    // Register callbacks
    void onPlayerJoin (OnPlayerJoin  cb) { m_onJoin   = cb; }
    void onPlayerLeave(OnPlayerLeave cb) { m_onLeave  = cb; }
    void onNodeUpdate (OnNodeUpdate  cb) { m_onUpdate = cb; }

    bool isConnected() const { return m_connected; }
    int  playerCount() const { return m_playerCount; }

private:
    bool          m_connected   = false;
    int           m_playerCount = 0;
    std::string   m_roomId;

    OnPlayerJoin  m_onJoin;
    OnPlayerLeave m_onLeave;
    OnNodeUpdate  m_onUpdate;
};
