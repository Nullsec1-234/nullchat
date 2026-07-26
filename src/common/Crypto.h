#pragma once

#include <string>
#include <vector>
#include <array>
#include <cstdint>

namespace chatter {

inline constexpr size_t AES_KEY_SIZE = 32;     // AES-256
inline constexpr size_t AES_IV_SIZE = 16;
inline constexpr size_t RSA_KEY_BITS = 4096;

struct KeyPair {
    std::string public_key;
    std::string private_key;
};

class Crypto {
public:
    Crypto() = default;

    KeyPair generateRSAKeyPair();
    std::string rsaEncrypt(const std::string& plaintext, const std::string& public_key_pem);
    std::string rsaDecrypt(const std::string& ciphertext, const std::string& private_key_pem);
    std::string rsaSign(const std::string& data, const std::string& private_key_pem);
    bool rsaVerify(const std::string& data, const std::string& signature,
                   const std::string& public_key_pem);

    // AES-GCM: authenticated symmetric encryption
    std::vector<uint8_t> aesEncrypt(const std::vector<uint8_t>& plaintext,
                                    const std::vector<uint8_t>& key);
    std::vector<uint8_t> aesDecrypt(const std::vector<uint8_t>& ciphertext,
                                    const std::vector<uint8_t>& key);

    std::vector<uint8_t> generateAESKey();
    std::vector<uint8_t> generateIV();

    std::string sha256(const std::string& data);

    // ECDH for P2P key exchange
    KeyPair generateECDHKeyPair();
    std::vector<uint8_t> ecdhDerive(const std::string& private_key_pem,
                                    const std::string& public_key_pem);

private:
    bool initialized = false;
    void ensureInitialized();
};

} // namespace chatter
