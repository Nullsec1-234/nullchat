#pragma once

#include <QObject>
#include <string>
#include <unordered_map>

namespace chatter {

class ClientCrypto;

struct P2PChannel {
    std::string peer_id;
    std::string peer_username;
    std::vector<uint8_t> session_key;
    bool connected = false;
};

class P2PManager : public QObject {
    Q_OBJECT
public:
    explicit P2PManager(ClientCrypto* crypto, QObject* parent = nullptr);

    bool hasSession(const std::string& peer_id) const;
    bool hasGroupKey(const std::string& group_id) const;
    std::string encryptMessage(const std::string& peer_id, const std::string& plaintext);
    std::string decryptMessage(const std::string& peer_id, const std::string& ciphertext);
    std::string encryptGroupMessage(const std::string& group_id, const std::string& plaintext);
    std::string decryptGroupMessage(const std::string& group_id, const std::string& ciphertext);

public slots:
    void handleOffer(const std::string& from_id, const std::string& sdp,
                     const std::string& public_key);

signals:
    void connected(const std::string& peer_id);

private:
    ClientCrypto* crypto_;
    std::unordered_map<std::string, P2PChannel> channels_;
    std::unordered_map<std::string, std::vector<uint8_t>> group_keys_;
};

} // namespace chatter
