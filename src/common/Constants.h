#pragma once

#include <cstdint>
#include <string_view>

namespace chatter {

inline constexpr int PROTOCOL_VERSION = 1;
inline constexpr int DEFAULT_PORT = 8447;
inline constexpr size_t MAX_MESSAGE_LEN = 65536;
inline constexpr size_t MAX_USERNAME_LEN = 32;
inline constexpr size_t MAX_GROUP_NAME_LEN = 64;

inline constexpr std::string_view APP_NAME = "chatter";

} // namespace chatter
