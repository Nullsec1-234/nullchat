#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <cstdint>

namespace chatter {

enum class MessageType : uint8_t {
    // Auth
    AuthLogin,
    AuthRegister,
    AuthResponse,

    // Messaging
    TextMessage,
    GroupMessage,

    // Group management
    CreateGroup,
    JoinGroup,
    LeaveGroup,
    GroupList,
    GroupInfo,

    // P2P signaling
    P2POffer,
    P2PAnswer,
    P2PICECandidate,

    // Status
    UserStatus,
    Ping,
    Pong,
    Error,

    // Presence
    UserOnline,
    UserOffline,

    // E2EE key exchange
    PubKeyRequest,
    PubKeyResponse,

    // Channel management
    RenameGroup,
    DeleteGroup,

    // Notifications
    Mention,
};

#pragma pack(push, 1)
struct PacketHeader {
    uint32_t length;
    uint8_t type;
    uint8_t version;
};
#pragma pack(pop)

struct Packet {
    MessageType type;
    std::string body;

    std::vector<uint8_t> serialize() const;
    static Packet deserialize(const std::vector<uint8_t>& data);
};

Packet make_packet(MessageType type, const std::string& body);

} // namespace chatter
