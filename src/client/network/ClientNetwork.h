#pragma once

#include <QObject>
#include <QTcpSocket>
#include <string>
#include <optional>
#include <utility>
#include <unordered_map>

namespace chatter {

struct ChatMessage;

class ClientNetwork : public QObject {
    Q_OBJECT
public:
    explicit ClientNetwork(QObject* parent = nullptr);
    ~ClientNetwork();

    bool connectAndLogin(const std::string& host, uint16_t port,
                          const std::string& username, const std::string& password,
                          const std::string& public_key = "");
    bool connectAndRegister(const std::string& host, uint16_t port,
                            const std::string& username, const std::string& password,
                            const std::string& public_key = "",
                            const std::string& invite_password = "");

    void connectToServer(const std::string& host, uint16_t port);
    void sendLogin(const std::string& username, const std::string& password);
    void sendRegister(const std::string& username, const std::string& password);
    void sendLoginOrRegister();
    void sendDirectMessage(const ChatMessage& msg);
    void sendGroupMessage(const ChatMessage& msg);
    void createGroup(const std::string& name, const std::string& encrypted_key);
    void joinGroup(const std::string& group_id, const std::string& encrypted_key);
    void sendP2POffer(const std::string& target_id, const std::string& public_key);
    void requestPublicKey(const std::string& user_id);
    void renameGroup(const std::string& group_id, const std::string& new_name);
    void deleteGroup(const std::string& group_id);

    void setPublicKey(const std::string& pubkey) { own_public_key_ = pubkey; }

    std::string userId() const { return user_id_; }
    std::string username() const { return username_; }
    bool isAuthenticated() const { return authenticated_; }
    bool isNull() const { return is_null_; }

    struct GroupEntry { std::string id; std::string name; };
    std::vector<GroupEntry> initialGroups() const { return initial_groups_; }

    std::string peerPublicKey(const std::string& peer_id) const {
        auto it = peer_keys_.find(peer_id);
        return it != peer_keys_.end() ? it->second : "";
    }

signals:
    void connected();
    void disconnected();
    void authSuccess(const std::string& user_id, const std::string& username);
    void authError(const std::string& error);
    void messageReceived(const ChatMessage& msg);
    void groupMessageReceived(const ChatMessage& msg);
    void userOnline(const std::string& user_id, const std::string& username);
    void userOffline(const std::string& user_id);
    void groupCreated(const std::string& group_id, const std::string& name);
    void p2pOfferReceived(const std::string& from_id, const std::string& sdp,
                          const std::string& public_key);
    void p2pAnswerReceived(const std::string& from_id, const std::string& sdp);
    void p2pICEReceived(const std::string& from_id, const std::string& candidate);
    void pubKeyReceived(const std::string& user_id, const std::string& public_key);
    void mentionReceived(const std::string& sender_id, const std::string& sender_name,
                         const std::string& group_id, const std::string& group_name,
                         const std::string& content);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();

private:
    void sendPacket(uint8_t type, const std::string& body);
    void handlePacket(uint8_t type, const std::string& body);

    QTcpSocket* socket_;
    std::string user_id_;
    std::string username_;
    std::string own_public_key_;
    bool authenticated_ = false;
    bool is_null_ = false;
    bool is_login_ = false;
    std::vector<GroupEntry> initial_groups_;
    std::string pending_username_;
    std::string pending_password_;
    QByteArray read_buffer_;
    std::unordered_map<std::string, std::string> peer_keys_;
};

} // namespace chatter
