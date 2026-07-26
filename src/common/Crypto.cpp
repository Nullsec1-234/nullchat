#include "Crypto.h"

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/rsa.h>
#include <openssl/ec.h>
#include <openssl/err.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/bn.h>

#include <cstring>
#include <stdexcept>
#include <sstream>

namespace chatter {

static std::string opensslError() {
    std::ostringstream oss;
    while (unsigned long err = ERR_get_error())
        oss << ERR_error_string(err, nullptr) << "; ";
    return oss.str();
}

static void check(bool ok, const char* msg) {
    if (!ok) throw std::runtime_error(std::string(msg) + ": " + opensslError());
}

void Crypto::ensureInitialized() {
    if (!initialized) {
        OpenSSL_add_all_algorithms();
        ERR_load_crypto_strings();
        initialized = true;
    }
}

static std::string bioToString(BIO* bio) {
    char* data;
    long len = BIO_get_mem_data(bio, &data);
    return std::string(data, len);
}

static std::string pemToKey(EVP_PKEY* pkey) {
    BIO* bio = BIO_new(BIO_s_mem());
    check(bio != nullptr, "BIO_new");
    check(PEM_write_bio_PrivateKey(bio, pkey, nullptr, nullptr, 0, nullptr, nullptr) == 1,
          "PEM_write_bio_PrivateKey");
    std::string result = bioToString(bio);
    BIO_free(bio);
    return result;
}

static std::string pemToPublicKey(EVP_PKEY* pkey) {
    BIO* bio = BIO_new(BIO_s_mem());
    check(bio != nullptr, "BIO_new");
    check(PEM_write_bio_PUBKEY(bio, pkey) == 1, "PEM_write_bio_PUBKEY");
    std::string result = bioToString(bio);
    BIO_free(bio);
    return result;
}

static EVP_PKEY* pemToEvpPkey(const std::string& pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    check(bio != nullptr, "BIO_new_mem_buf");
    EVP_PKEY* pkey = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    check(pkey != nullptr, "PEM_read_bio_PrivateKey");
    return pkey;
}

static EVP_PKEY* pemToEvpPublicKey(const std::string& pem) {
    BIO* bio = BIO_new_mem_buf(pem.data(), static_cast<int>(pem.size()));
    check(bio != nullptr, "BIO_new_mem_buf");
    EVP_PKEY* pkey = PEM_read_bio_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    check(pkey != nullptr, "PEM_read_bio_PUBKEY");
    return pkey;
}

KeyPair Crypto::generateRSAKeyPair() {
    ensureInitialized();
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_RSA, nullptr);
    check(ctx != nullptr, "EVP_PKEY_CTX_new_id");
    check(EVP_PKEY_keygen_init(ctx) == 1, "EVP_PKEY_keygen_init");
    check(EVP_PKEY_CTX_set_rsa_keygen_bits(ctx, RSA_KEY_BITS) == 1,
          "EVP_PKEY_CTX_set_rsa_keygen_bits");
    EVP_PKEY* pkey = nullptr;
    check(EVP_PKEY_keygen(ctx, &pkey) == 1, "EVP_PKEY_keygen");
    EVP_PKEY_CTX_free(ctx);

    KeyPair kp;
    kp.private_key = pemToKey(pkey);
    kp.public_key = pemToPublicKey(pkey);
    EVP_PKEY_free(pkey);
    return kp;
}

std::string Crypto::rsaEncrypt(const std::string& plaintext, const std::string& public_key_pem) {
    ensureInitialized();
    EVP_PKEY* pkey = pemToEvpPublicKey(public_key_pem);
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    check(ctx != nullptr, "EVP_PKEY_CTX_new");
    check(EVP_PKEY_encrypt_init(ctx) == 1, "EVP_PKEY_encrypt_init");
    check(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) == 1,
          "EVP_PKEY_CTX_set_rsa_padding");

    size_t outlen;
    check(EVP_PKEY_encrypt(ctx, nullptr, &outlen,
          reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size()) == 1,
          "EVP_PKEY_encrypt (size)");
    std::string result(outlen, '\0');
    check(EVP_PKEY_encrypt(ctx, reinterpret_cast<uint8_t*>(result.data()), &outlen,
          reinterpret_cast<const uint8_t*>(plaintext.data()), plaintext.size()) == 1,
          "EVP_PKEY_encrypt");
    result.resize(outlen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return result;
}

std::string Crypto::rsaDecrypt(const std::string& ciphertext, const std::string& private_key_pem) {
    ensureInitialized();
    EVP_PKEY* pkey = pemToEvpPkey(private_key_pem);
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(pkey, nullptr);
    check(ctx != nullptr, "EVP_PKEY_CTX_new");
    check(EVP_PKEY_decrypt_init(ctx) == 1, "EVP_PKEY_decrypt_init");
    check(EVP_PKEY_CTX_set_rsa_padding(ctx, RSA_PKCS1_OAEP_PADDING) == 1,
          "EVP_PKEY_CTX_set_rsa_padding");

    size_t outlen;
    check(EVP_PKEY_decrypt(ctx, nullptr, &outlen,
          reinterpret_cast<const uint8_t*>(ciphertext.data()), ciphertext.size()) == 1,
          "EVP_PKEY_decrypt (size)");
    std::string result(outlen, '\0');
    check(EVP_PKEY_decrypt(ctx, reinterpret_cast<uint8_t*>(result.data()), &outlen,
          reinterpret_cast<const uint8_t*>(ciphertext.data()), ciphertext.size()) == 1,
          "EVP_PKEY_decrypt");
    result.resize(outlen);
    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(pkey);
    return result;
}

std::string Crypto::rsaSign(const std::string& data, const std::string& private_key_pem) {
    ensureInitialized();
    EVP_PKEY* pkey = pemToEvpPkey(private_key_pem);
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    check(mdctx != nullptr, "EVP_MD_CTX_new");
    check(EVP_DigestSignInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) == 1,
          "EVP_DigestSignInit");

    size_t siglen;
    check(EVP_DigestSign(mdctx, nullptr, &siglen,
          reinterpret_cast<const uint8_t*>(data.data()), data.size()) == 1,
          "EVP_DigestSign (size)");
    std::string result(siglen, '\0');
    check(EVP_DigestSign(mdctx, reinterpret_cast<uint8_t*>(result.data()), &siglen,
          reinterpret_cast<const uint8_t*>(data.data()), data.size()) == 1,
          "EVP_DigestSign");
    result.resize(siglen);

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    return result;
}

bool Crypto::rsaVerify(const std::string& data, const std::string& signature,
                       const std::string& public_key_pem) {
    ensureInitialized();
    EVP_PKEY* pkey = pemToEvpPublicKey(public_key_pem);
    EVP_MD_CTX* mdctx = EVP_MD_CTX_new();
    check(mdctx != nullptr, "EVP_MD_CTX_new");
    check(EVP_DigestVerifyInit(mdctx, nullptr, EVP_sha256(), nullptr, pkey) == 1,
          "EVP_DigestVerifyInit");

    int rc = EVP_DigestVerify(mdctx,
        reinterpret_cast<const uint8_t*>(signature.data()), signature.size(),
        reinterpret_cast<const uint8_t*>(data.data()), data.size());

    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(pkey);
    return rc == 1;
}

static std::vector<uint8_t> aesEncryptDecrypt(const std::vector<uint8_t>& input,
                                               const std::vector<uint8_t>& key,
                                               const std::vector<uint8_t>& iv,
                                               bool encrypt) {
    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    check(ctx != nullptr, "EVP_CIPHER_CTX_new");

    int rc;
    if (encrypt)
        rc = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data());
    else
        rc = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, key.data(), iv.data());
    check(rc == 1, "EVP_EncryptInit_ex/EVP_DecryptInit_ex");

    std::vector<uint8_t> output(input.size() + 16);
    int outlen = 0, tmplen = 0;

    if (encrypt) {
        check(EVP_EncryptUpdate(ctx, output.data(), &outlen, input.data(), input.size()) == 1,
              "EVP_EncryptUpdate");
        check(EVP_EncryptFinal_ex(ctx, output.data() + outlen, &tmplen) == 1,
              "EVP_EncryptFinal_ex");
        outlen += tmplen;

        uint8_t tag[16];
        check(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag) == 1,
              "EVP_CTRL_GCM_GET_TAG");
        output.resize(outlen + 16);
        std::memcpy(output.data() + outlen, tag, 16);
    } else {
        if (input.size() < 16)
            throw std::runtime_error("ciphertext too short for GCM tag");
        size_t ctlen = input.size() - 16;

        uint8_t tag[16];
        std::memcpy(tag, input.data() + ctlen, 16);
        check(EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag) == 1,
              "EVP_CTRL_GCM_SET_TAG");

        check(EVP_DecryptUpdate(ctx, output.data(), &outlen, input.data(), ctlen) == 1,
              "EVP_DecryptUpdate");
        int rc2 = EVP_DecryptFinal_ex(ctx, output.data() + outlen, &tmplen);
        if (rc2 <= 0)
            throw std::runtime_error("AES-GCM decryption failed (tampered data)");
        outlen += tmplen;
    }

    output.resize(outlen);
    EVP_CIPHER_CTX_free(ctx);
    return output;
}

std::vector<uint8_t> Crypto::aesEncrypt(const std::vector<uint8_t>& plaintext,
                                        const std::vector<uint8_t>& key) {
    auto iv = generateIV();
    std::vector<uint8_t> iv_tag(iv.begin(), iv.end());
    auto ciphertext = aesEncryptDecrypt(plaintext, key, iv, true);
    iv_tag.insert(iv_tag.end(), ciphertext.begin(), ciphertext.end());
    return iv_tag;  // [IV(16) | ciphertext + tag]
}

std::vector<uint8_t> Crypto::aesDecrypt(const std::vector<uint8_t>& data,
                                        const std::vector<uint8_t>& key) {
    if (data.size() < AES_IV_SIZE)
        throw std::runtime_error("data too short");
    std::vector<uint8_t> iv(data.begin(), data.begin() + AES_IV_SIZE);
    std::vector<uint8_t> ct(data.begin() + AES_IV_SIZE, data.end());
    return aesEncryptDecrypt(ct, key, iv, false);
}

std::vector<uint8_t> Crypto::generateAESKey() {
    std::vector<uint8_t> key(AES_KEY_SIZE);
    check(RAND_bytes(key.data(), AES_KEY_SIZE) == 1, "RAND_bytes AES key");
    return key;
}

std::vector<uint8_t> Crypto::generateIV() {
    std::vector<uint8_t> iv(AES_IV_SIZE);
    check(RAND_bytes(iv.data(), AES_IV_SIZE) == 1, "RAND_bytes IV");
    return iv;
}

std::string Crypto::sha256(const std::string& data) {
    uint8_t hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const uint8_t*>(data.data()), data.size(), hash);
    std::ostringstream oss;
    for (auto b : hash)
        oss << std::hex << (b >> 4) << (b & 0xf);
    return oss.str();
}

KeyPair Crypto::generateECDHKeyPair() {
    ensureInitialized();
    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
    check(ctx != nullptr, "EVP_PKEY_CTX_new_id");
    check(EVP_PKEY_keygen_init(ctx) == 1, "EVP_PKEY_keygen_init");
    check(EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ctx, NID_X9_62_prime256v1) == 1,
          "EVP_PKEY_CTX_set_ec_paramgen_curve_nid");
    EVP_PKEY* pkey = nullptr;
    check(EVP_PKEY_keygen(ctx, &pkey) == 1, "EVP_PKEY_keygen");
    EVP_PKEY_CTX_free(ctx);

    KeyPair kp;
    kp.private_key = pemToKey(pkey);
    kp.public_key = pemToPublicKey(pkey);
    EVP_PKEY_free(pkey);
    return kp;
}

std::vector<uint8_t> Crypto::ecdhDerive(const std::string& private_key_pem,
                                        const std::string& public_key_pem) {
    ensureInitialized();
    EVP_PKEY* priv = pemToEvpPkey(private_key_pem);
    EVP_PKEY* pub = pemToEvpPublicKey(public_key_pem);

    EVP_PKEY_CTX* ctx = EVP_PKEY_CTX_new(priv, nullptr);
    check(ctx != nullptr, "EVP_PKEY_CTX_new");
    check(EVP_PKEY_derive_init(ctx) == 1, "EVP_PKEY_derive_init");
    check(EVP_PKEY_derive_set_peer(ctx, pub) == 1, "EVP_PKEY_derive_set_peer");

    size_t secret_len;
    check(EVP_PKEY_derive(ctx, nullptr, &secret_len) == 1, "EVP_PKEY_derive (size)");
    std::vector<uint8_t> secret(secret_len);
    check(EVP_PKEY_derive(ctx, secret.data(), &secret_len) == 1, "EVP_PKEY_derive");
    secret.resize(secret_len);

    EVP_PKEY_CTX_free(ctx);
    EVP_PKEY_free(priv);
    EVP_PKEY_free(pub);

    // Hash the shared secret to get a fixed-size AES key
    std::vector<uint8_t> aes_key(SHA256_DIGEST_LENGTH);
    SHA256(secret.data(), secret.size(), aes_key.data());
    aes_key.resize(AES_KEY_SIZE);
    return aes_key;
}

} // namespace chatter
