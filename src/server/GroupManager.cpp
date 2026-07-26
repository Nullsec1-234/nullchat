#include "GroupManager.h"
#include "Session.h"
#include "Database.h"

namespace chatter {

GroupManager::GroupManager(Database* db) : db_(db) {}

std::string GroupManager::createGroup(const std::string& name, const std::string& owner_id,
                                      const std::string& encrypted_key) {
    return db_->createGroup(name, owner_id, encrypted_key);
}

bool GroupManager::addMember(const std::string& group_id, const std::string& user_id,
                             const std::string& encrypted_key) {
    return db_->addMember(group_id, user_id, encrypted_key);
}

std::vector<GroupInfo> GroupManager::getUserGroups(const std::string& user_id) {
    auto records = db_->getUserGroups(user_id);
    std::vector<GroupInfo> groups;
    for (auto& r : records) {
        GroupInfo g;
        g.id = r.id;
        g.name = r.name;
        g.member_ids = db_->getGroupMembers(r.id);
        groups.push_back(std::move(g));
    }
    return groups;
}

void GroupManager::broadcastToGroup(const std::string& group_id, const std::string& sender_id,
                                    const Packet& pkt, SessionFinder find_session) {
    auto members = db_->getGroupMembers(group_id);
    auto data = pkt.serialize();
    for (auto& member_id : members) {
        if (member_id == sender_id) continue;
        if (auto* session = find_session(member_id))
            session->sendRaw(data);
    }
}

} // namespace chatter
