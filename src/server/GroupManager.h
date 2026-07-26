#pragma once

#include "../common/Protocol.h"
#include "../common/Message.h"
#include <string>
#include <vector>
#include <functional>

namespace chatter {

class Database;
class Session;

class GroupManager {
public:
    using SessionFinder = std::function<Session*(const std::string&)>;

    explicit GroupManager(Database* db);

    std::string createGroup(const std::string& name, const std::string& owner_id,
                            const std::string& encrypted_key);
    bool addMember(const std::string& group_id, const std::string& user_id,
                   const std::string& encrypted_key);
    std::vector<GroupInfo> getUserGroups(const std::string& user_id);
    void broadcastToGroup(const std::string& group_id, const std::string& sender_id,
                          const Packet& pkt, SessionFinder find_session);

private:
    Database* db_;
};

} // namespace chatter
