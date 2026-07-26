#include <gtest/gtest.h>
#include "../src/common/Protocol.h"

using namespace chatter;

TEST(ProtocolTest, SerializeDeserialize) {
    Packet pkt;
    pkt.type = MessageType::TextMessage;
    pkt.body = "{\"hello\":\"world\"}";

    auto data = pkt.serialize();
    auto pkt2 = Packet::deserialize(data);

    EXPECT_EQ(pkt.type, pkt2.type);
    EXPECT_EQ(pkt.body, pkt2.body);
}

TEST(ProtocolTest, MakePacket) {
    auto pkt = make_packet(MessageType::Ping, "{}");
    EXPECT_EQ(pkt.type, MessageType::Ping);
    EXPECT_EQ(pkt.body, "{}");
}
