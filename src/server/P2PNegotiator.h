#pragma once

#include <string>

namespace chatter {

// The server acts as a signaling relay for P2P connections.
// Actual P2P data flows directly between clients after negotiation.
class P2PNegotiator {
public:
    P2PNegotiator() = default;
    ~P2PNegotiator() = default;
};

} // namespace chatter
