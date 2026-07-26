#include <gtest/gtest.h>
#include "../src/common/Crypto.h"

using namespace chatter;

TEST(CryptoTest, RSAEncryptDecrypt) {
    Crypto crypto;
    auto kp = crypto.generateRSAKeyPair();

    std::string plaintext = "hello chatter";
    auto ciphertext = crypto.rsaEncrypt(plaintext, kp.public_key);
    auto decrypted = crypto.rsaDecrypt(ciphertext, kp.private_key);

    EXPECT_EQ(plaintext, decrypted);
}

TEST(CryptoTest, RSASignVerify) {
    Crypto crypto;
    auto kp = crypto.generateRSAKeyPair();

    std::string data = "message to sign";
    auto sig = crypto.rsaSign(data, kp.private_key);
    EXPECT_TRUE(crypto.rsaVerify(data, sig, kp.public_key));
}

TEST(CryptoTest, AESEncryptDecrypt) {
    Crypto crypto;
    auto key = crypto.generateAESKey();
    std::vector<uint8_t> plaintext = {'h', 'e', 'l', 'l', 'o'};

    auto ct = crypto.aesEncrypt(plaintext, key);
    auto pt = crypto.aesDecrypt(ct, key);

    EXPECT_EQ(plaintext, pt);
}

TEST(CryptoTest, SHA256) {
    Crypto crypto;
    auto hash = crypto.sha256("hello");
    EXPECT_EQ(hash.size(), 64); // hex-encoded SHA-256
}

TEST(CryptoTest, ECDHDerive) {
    Crypto crypto;
    auto alice = crypto.generateECDHKeyPair();
    auto bob = crypto.generateECDHKeyPair();

    auto alice_shared = crypto.ecdhDerive(alice.private_key, bob.public_key);
    auto bob_shared = crypto.ecdhDerive(bob.private_key, alice.public_key);

    EXPECT_EQ(alice_shared, bob_shared);
}
