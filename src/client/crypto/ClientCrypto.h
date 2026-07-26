#pragma once

#include "../../common/Crypto.h"
#include <unordered_map>

namespace chatter {

class ClientCrypto : public Crypto {
public:
    ClientCrypto();

    std::string getPublicKey() const { return keypair_.public_key; }
    std::string getPrivateKey() const { return keypair_.private_key; }
    std::string getECPublicKey() const { return ecdh_keypair_.public_key; }

    KeyPair generateECDH();
    std::vector<uint8_t> generateGroupKey();
    std::string encryptWithOwnKey(const std::vector<uint8_t>& key);

    std::vector<uint8_t> deriveSharedKey(const std::string& peer_pubkey);
    std::vector<uint8_t> encryptDM(const std::string& plaintext, const std::vector<uint8_t>& key);
    std::string decryptDM(const std::vector<uint8_t>& ciphertext, const std::vector<uint8_t>& key);

    std::vector<uint8_t> getOrDeriveKey(const std::string& peer_id, const std::string& peer_pubkey);

private:
    KeyPair keypair_;
    KeyPair ecdh_keypair_;
    std::unordered_map<std::string, std::vector<uint8_t>> shared_keys_;
};

} // namespace chatter
