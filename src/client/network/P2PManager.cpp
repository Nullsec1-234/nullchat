#include "P2PManager.h"
#include "../crypto/ClientCrypto.h"

#include <QDebug>

namespace chatter {

P2PManager::P2PManager(ClientCrypto* crypto, QObject* parent)
    : QObject(parent), crypto_(crypto) {}

bool P2PManager::hasSession(const std::string& peer_id) const {
    auto it = channels_.find(peer_id);
    return it != channels_.end() && it->second.connected;
}

bool P2PManager::hasGroupKey(const std::string& group_id) const {
    return group_keys_.find(group_id) != group_keys_.end();
}

std::string P2PManager::encryptMessage(const std::string& peer_id, const std::string& plaintext) {
    auto it = channels_.find(peer_id);
    if (it == channels_.end()) return plaintext;
    auto ct = crypto_->aesEncrypt(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
        it->second.session_key);
    return std::string(ct.begin(), ct.end());
}

std::string P2PManager::decryptMessage(const std::string& peer_id, const std::string& ciphertext) {
    auto it = channels_.find(peer_id);
    if (it == channels_.end()) return ciphertext;
    auto pt = crypto_->aesDecrypt(
        std::vector<uint8_t>(ciphertext.begin(), ciphertext.end()),
        it->second.session_key);
    return std::string(pt.begin(), pt.end());
}

std::string P2PManager::encryptGroupMessage(const std::string& group_id,
                                            const std::string& plaintext) {
    auto it = group_keys_.find(group_id);
    if (it == group_keys_.end()) return plaintext;
    auto ct = crypto_->aesEncrypt(
        std::vector<uint8_t>(plaintext.begin(), plaintext.end()),
        it->second);
    return std::string(ct.begin(), ct.end());
}

std::string P2PManager::decryptGroupMessage(const std::string& group_id,
                                            const std::string& ciphertext) {
    auto it = group_keys_.find(group_id);
    if (it == group_keys_.end()) return ciphertext;
    auto pt = crypto_->aesDecrypt(
        std::vector<uint8_t>(ciphertext.begin(), ciphertext.end()),
        it->second);
    return std::string(pt.begin(), pt.end());
}

void P2PManager::handleOffer(const std::string& from_id, const std::string& sdp,
                             const std::string& public_key) {
    Q_UNUSED(sdp);

    // ECDH key exchange
    auto my_keypair = crypto_->generateECDH();
    auto shared_secret = crypto_->ecdhDerive(my_keypair.private_key, public_key);

    P2PChannel ch;
    ch.peer_id = from_id;
    ch.session_key = shared_secret;
    ch.connected = true;
    channels_[from_id] = ch;

    emit connected(from_id);
}

} // namespace chatter
