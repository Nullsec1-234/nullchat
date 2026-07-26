#pragma once

#include <string>
#include <chrono>
#include <vector>
#include <cstdint>

namespace chatter {

struct UserInfo {
    std::string id;
    std::string username;
    std::string public_key;
    bool online = false;
};

struct GroupInfo {
    std::string id;
    std::string name;
    std::vector<std::string> member_ids;
    std::string encrypted_symmetric_key;
};

struct ChatMessage {
    std::string id;
    std::string sender_id;
    std::string sender_name;
    std::string content;          // plaintext or ciphertext
    std::string group_id;         // empty for DMs
    std::string recipient_id;     // for DMs
    bool encrypted = false;
    int64_t timestamp = 0;

    int64_t now() {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

struct P2PSession {
    std::string peer_id;
    std::string peer_username;
    std::string peer_address;
    uint16_t peer_port = 0;
    std::string session_key;
    std::string remote_public_key;
    bool connected = false;
};

} // namespace chatter
