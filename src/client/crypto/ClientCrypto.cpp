#include "ClientCrypto.h"

namespace chatter {

ClientCrypto::ClientCrypto() {
    keypair_ = generateRSAKeyPair();
    ecdh_keypair_ = generateECDHKeyPair();
}

KeyPair ClientCrypto::generateECDH() {
    ecdh_keypair_ = generateECDHKeyPair();
    return ecdh_keypair_;
}

std::vector<uint8_t> ClientCrypto::generateGroupKey() {
    return generateAESKey();
}

std::string ClientCrypto::encryptWithOwnKey(const std::vector<uint8_t>& key) {
    std::string key_str(key.begin(), key.end());
    return rsaEncrypt(key_str, keypair_.public_key);
}

std::vector<uint8_t> ClientCrypto::deriveSharedKey(const std::string& peer_pubkey) {
    return ecdhDerive(ecdh_keypair_.private_key, peer_pubkey);
}

std::vector<uint8_t> ClientCrypto::encryptDM(const std::string& plaintext,
                                              const std::vector<uint8_t>& key) {
    std::vector<uint8_t> pt(plaintext.begin(), plaintext.end());
    return aesEncrypt(pt, key);
}

std::string ClientCrypto::decryptDM(const std::vector<uint8_t>& ciphertext,
                                     const std::vector<uint8_t>& key) {
    auto pt = aesDecrypt(ciphertext, key);
    return std::string(pt.begin(), pt.end());
}

std::vector<uint8_t> ClientCrypto::getOrDeriveKey(const std::string& peer_id,
                                                   const std::string& peer_pubkey) {
    auto it = shared_keys_.find(peer_id);
    if (it != shared_keys_.end())
        return it->second;
    auto key = deriveSharedKey(peer_pubkey);
    shared_keys_[peer_id] = key;
    return key;
}

} // namespace chatter
