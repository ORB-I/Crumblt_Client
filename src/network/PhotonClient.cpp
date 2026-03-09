#include "PhotonClient.h"
#include <LoadBalancing-cpp/inc/Client.h>
#include <Common-cpp/inc/Common.h>
#include <cstdio>

#define PHOTON_APP_ID "4b063a64-14bb-4a04-a97e-46b4ee29e9c9"

static constexpr nByte EV_NODE_UPDATE = 0;
static constexpr nByte EV_PLAYER_MOVE = 1;
static constexpr nByte EV_CHAT        = 2;

using namespace ExitGames::LoadBalancing;
using namespace ExitGames::Common;

class PhotonClientImpl : public Listener {
public:
    PhotonClientImpl(PhotonClient& owner, const std::string& username)
        : m_owner(owner)
        , m_lbc(*this, PHOTON_APP_ID, "1.0")
        , m_username(username)
    {}

    void connect(const std::string& gameId) {
        m_roomId = JString("crumblt-game-") + gameId.c_str();
        AuthenticationValues auth;
        auth.setUserID(JString(m_username.c_str()));
        m_lbc.connect(auth);
    }

    void disconnect() { m_lbc.disconnect(); }
    void update()     { m_lbc.service(); }

    void sendPlayerMove(float x, float z) {
        if (!m_lbc.getIsInGameRoom()) return;
        Hashtable ev;
        ev.put((nByte)0, x);
        ev.put((nByte)1, z);
        m_lbc.opRaiseEvent(false, ev, EV_PLAYER_MOVE);
    }

    void sendNodeUpdate(const std::string& nodeId, float x, float y, float z) {
        if (!m_lbc.getIsInGameRoom()) return;
        Hashtable pos;
        pos.put((nByte)0, x);
        pos.put((nByte)1, y);
        pos.put((nByte)2, z);
        Hashtable ev;
        ev.put((nByte)0, JString(nodeId.c_str()));
        ev.put((nByte)1, pos);
        m_lbc.opRaiseEvent(false, ev, EV_NODE_UPDATE);
    }

    void sendChat(const std::string& message, const std::string& username) {
        if (!m_lbc.getIsInGameRoom()) return;
        Hashtable ev;
        ev.put((nByte)0, JString(username.c_str()));
        ev.put((nByte)1, JString(message.c_str()));
        m_lbc.opRaiseEvent(true, ev, EV_CHAT);
    }

private:
    void debugReturn(int, const JString& msg) override {
        printf("[Photon] %s\n", msg.UTF8Representation().cstr());
    }
    void connectionErrorReturn(int errorCode) override {
        printf("[Photon] Connection error: %d\n", errorCode);
    }
    void clientErrorReturn(int errorCode) override {
        printf("[Photon] Client error: %d\n", errorCode);
    }
    void warningReturn(int warningCode) override {
        printf("[Photon] Warning: %d\n", warningCode);
    }
    void serverErrorReturn(int errorCode) override {
        printf("[Photon] Server error: %d\n", errorCode);
    }

    void connectReturn(int errorCode, const JString& errorString, const JString& region, const JString& cluster) override {
        if (errorCode) {
            printf("[Photon] connectReturn error %d: %s\n", errorCode, errorString.UTF8Representation().cstr());
            return;
        }
        printf("[Photon] Connected [%s/%s], joining %s\n",
            region.UTF8Representation().cstr(),
            cluster.UTF8Representation().cstr(),
            m_roomId.UTF8Representation().cstr());
        RoomOptions opts;
        opts.setMaxPlayers(20);
        m_lbc.opJoinOrCreateRoom(m_roomId, opts);
    }

    void disconnectReturn() override {
        m_owner.m_connected   = false;
        m_owner.m_playerCount = 0;
        printf("[Photon] Disconnected\n");
    }

    void joinRoomEventAction(int playerNr, const JVector<int>&, const Player& player) override {
        if (playerNr == m_lbc.getLocalPlayer().getNumber()) return;
        m_owner.m_playerCount++;
        if (m_owner.m_onJoin) {
            std::string uname = player.getName().UTF8Representation().cstr();
            m_owner.m_onJoin(playerNr, uname);
        }
    }

    void leaveRoomEventAction(int playerNr, bool) override {
        m_owner.m_playerCount--;
        if (m_owner.m_onLeave)
            m_owner.m_onLeave(playerNr);
    }

    void customEventAction(int playerNr, nByte eventCode, const Object& eventContent) override {
        Hashtable data = ValueObject<Hashtable>(eventContent).getDataCopy();

        if (eventCode == EV_PLAYER_MOVE && m_owner.m_onMove) {
            float x = ValueObject<float>(data.getValue((nByte)0)).getDataCopy();
            float z = ValueObject<float>(data.getValue((nByte)1)).getDataCopy();
            m_owner.m_onMove(playerNr, x, z);
        }
        else if (eventCode == EV_NODE_UPDATE && m_owner.m_onUpdate) {
            JString nodeIdJ = ValueObject<JString>(data.getValue((nByte)0)).getDataCopy();
            std::string nodeId = nodeIdJ.UTF8Representation().cstr();
            Hashtable pos = ValueObject<Hashtable>(data.getValue((nByte)1)).getDataCopy();
            float x = ValueObject<float>(pos.getValue((nByte)0)).getDataCopy();
            float y = ValueObject<float>(pos.getValue((nByte)1)).getDataCopy();
            float z = ValueObject<float>(pos.getValue((nByte)2)).getDataCopy();
            m_owner.m_onUpdate(nodeId, x, y, z);
        }
        else if (eventCode == EV_CHAT && m_owner.m_onChat) {
            std::string uname = ValueObject<JString>(data.getValue((nByte)0)).getDataCopy().UTF8Representation().cstr();
            std::string msg   = ValueObject<JString>(data.getValue((nByte)1)).getDataCopy().UTF8Representation().cstr();
            m_owner.m_onChat(uname, msg);
        }
    }

    void joinOrCreateRoomReturn(int localPlayerNr, const Hashtable&, const Hashtable&, int errorCode, const JString& errorString) override {
        if (errorCode) {
            printf("[Photon] joinOrCreateRoom error %d: %s\n", errorCode, errorString.UTF8Representation().cstr());
            return;
        }
        m_owner.m_connected   = true;
        m_owner.m_playerCount = m_lbc.getCurrentlyJoinedRoom().getPlayerCount();
        printf("[Photon] Joined room as player %d, %d players total\n", localPlayerNr, m_owner.m_playerCount);

        // Spawn players already in the room
        const JVector<Player*>& players = m_lbc.getCurrentlyJoinedRoom().getPlayers();
        for (unsigned int i = 0; i < players.getSize(); i++) {
            const Player* p = players[i];
            if (p->getNumber() == localPlayerNr) continue;
            if (m_owner.m_onJoin)
                m_owner.m_onJoin(p->getNumber(), p->getName().UTF8Representation().cstr());
        }
    }

    void leaveRoomReturn(int errorCode, const JString& errorString) override {
        if (errorCode)
            printf("[Photon] leaveRoom error %d: %s\n", errorCode, errorString.UTF8Representation().cstr());
    }

    PhotonClient&  m_owner;
    Client         m_lbc;
    JString        m_roomId;
    std::string    m_username;
};

// ---- PhotonClient ----
PhotonClient::PhotonClient() {}

PhotonClient::~PhotonClient() {
    disconnect();
    delete m_impl;
}

void PhotonClient::connect(const std::string& gameId, const std::string& username) {
    m_username = username;
    if (!m_impl)
        m_impl = new PhotonClientImpl(*this, username);
    m_impl->connect(gameId);
}

void PhotonClient::disconnect() {
    if (m_impl) m_impl->disconnect();
}

void PhotonClient::update() {
    if (m_impl) m_impl->update();
}

void PhotonClient::sendPlayerMove(float x, float z) {
    if (m_impl && m_connected) m_impl->sendPlayerMove(x, z);
}

void PhotonClient::sendNodeUpdate(const std::string& nodeId, float x, float y, float z) {
    if (m_impl && m_connected) m_impl->sendNodeUpdate(nodeId, x, y, z);
}

void PhotonClient::sendChat(const std::string& message) {
    if (m_impl && m_connected) m_impl->sendChat(message, m_username);
}
