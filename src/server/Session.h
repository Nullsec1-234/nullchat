#pragma once

#include <QObject>
#include <QByteArray>
#include <string>
#include <functional>
#include <vector>

class QSslSocket;

namespace chatter {

class Database;
class GroupManager;
class P2PNegotiator;

class Session : public QObject {
    Q_OBJECT
public:
    using SessionFinder = std::function<Session*(const std::string&)>;

    Session(QSslSocket* socket, Database* db,
            GroupManager* gm, P2PNegotiator* p2p,
            SessionFinder find_session,
            QObject* parent = nullptr);
    ~Session();

    std::string userId() const { return user_id_; }
    std::string username() const { return username_; }
    bool isAuthenticated() const { return authenticated_; }
    void sendPacket(uint8_t type, const std::string& body);
    void sendRaw(const std::vector<uint8_t>& data);

signals:
    void authenticated(const std::string& user_id);
    void disconnected(const std::string& user_id);

private slots:
    void onReadyRead();
    void onDisconnected();

private:
    void handlePacket(uint8_t type, const std::string& body);
    void handleAuthLogin(const std::string& json);
    void handleAuthRegister(const std::string& json);
    void handleTextMessage(const std::string& json);
    void handleGroupMessage(const std::string& json);
    void handleCreateGroup(const std::string& json);
    void handleJoinGroup(const std::string& json);
    void handleP2POffer(const std::string& json);
    void handleP2PAnswer(const std::string& json);
    void handleP2PICECandidate(const std::string& json);

    QSslSocket* socket_;
    Database* db_;
    GroupManager* gm_;
    P2PNegotiator* p2p_;
    SessionFinder find_session_;
    std::string user_id_;
    std::string username_;
    bool authenticated_ = false;
    QByteArray read_buffer_;
};

} // namespace chatter
