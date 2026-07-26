#include "Protocol.h"
#include "Constants.h"
#include <QtEndian>
#include <cstring>
#include <stdexcept>

namespace chatter {

Packet make_packet(MessageType type, const std::string& body) {
    return Packet{type, body};
}

std::vector<uint8_t> Packet::serialize() const {
    PacketHeader hdr;
    hdr.version = PROTOCOL_VERSION;
    hdr.type = static_cast<uint8_t>(type);
    hdr.length = qToBigEndian(static_cast<uint32_t>(body.size()));

    std::vector<uint8_t> data(sizeof(PacketHeader) + body.size());
    std::memcpy(data.data(), &hdr, sizeof(PacketHeader));
    std::memcpy(data.data() + sizeof(PacketHeader), body.data(), body.size());
    return data;
}

Packet Packet::deserialize(const std::vector<uint8_t>& data) {
    if (data.size() < sizeof(PacketHeader))
        throw std::runtime_error("packet too small");

    PacketHeader hdr;
    std::memcpy(&hdr, data.data(), sizeof(PacketHeader));

    if (hdr.version != PROTOCOL_VERSION)
        throw std::runtime_error("protocol version mismatch");

    Packet pkt;
    pkt.type = static_cast<MessageType>(hdr.type);
    auto body_len = qFromBigEndian(hdr.length);
    pkt.body.assign(reinterpret_cast<const char*>(data.data() + sizeof(PacketHeader)),
                    body_len);
    return pkt;
}

} // namespace chatter
