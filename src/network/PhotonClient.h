#pragma once
#include <string>
#include <functional>
#include <cstdint>

class PhotonClientImpl;

using OnPlayerJoin   = std::function<void(int actorNr, const std::string& username)>;
using OnPlayerLeave  = std::function<void(int actorNr)>;
using OnPlayerMove   = std::function<void(int actorNr, float x, float z)>;
using OnNodeUpdate   = std::function<void(const std::string& nodeId, float x, float y, float z)>;

class PhotonClient {
public:
    PhotonClient();
    ~PhotonClient();

    void connect(const std::string& gameId, const std::string& username);
    void disconnect();
    void update();

    // Send local player position to all peers
    void sendPlayerMove(float x, float z);
    void sendNodeUpdate(const std::string& nodeId, float x, float y, float z);

    void onPlayerJoin (OnPlayerJoin  cb) { m_onJoin   = cb; }
    void onPlayerLeave(OnPlayerLeave cb) { m_onLeave  = cb; }
    void onPlayerMove (OnPlayerMove  cb) { m_onMove   = cb; }
    void onNodeUpdate (OnNodeUpdate  cb) { m_onUpdate = cb; }

    bool isConnected() const { return m_connected; }
    int  playerCount() const { return m_playerCount; }

private:
    friend class PhotonClientImpl;

    bool m_connected   = false;
    int  m_playerCount = 0;

    OnPlayerJoin  m_onJoin;
    OnPlayerLeave m_onLeave;
    OnPlayerMove  m_onMove;
    OnNodeUpdate  m_onUpdate;

    PhotonClientImpl* m_impl = nullptr;
};
