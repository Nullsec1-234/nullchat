#pragma once

#include <QSqlDatabase>
#include <string>
#include <tuple>
#include <vector>

namespace chatter {

struct UserRecord {
    std::string id;
    std::string username;
    std::string password_hash;
    std::string public_key;
};

struct GroupRecord {
    std::string id;
    std::string name;
    std::string owner_id;
};

struct MemberRecord {
    std::string user_id;
    std::string group_id;
    std::string encrypted_symmetric_key;
};

class Database {
public:
    Database();
    ~Database();

    bool open(const std::string& path);
    void close();

    std::tuple<bool, std::string> registerUser(const std::string& username,
                                                const std::string& password);
    std::tuple<bool, std::string> loginUser(const std::string& username,
                                            const std::string& password);
    std::string getPublicKey(const std::string& user_id);

    std::string createGroup(const std::string& name, const std::string& owner_id,
                            const std::string& encrypted_key);
    bool addMember(const std::string& group_id, const std::string& user_id,
                   const std::string& encrypted_key);
    std::vector<std::string> getGroupMembers(const std::string& group_id);
    std::vector<GroupRecord> getUserGroups(const std::string& user_id);

    void storeMessage(const std::string& sender_id, const std::string& group_id,
                      const std::string& content, bool encrypted);

private:
    bool createTables();
    QSqlDatabase db_;
    std::string db_path_;
};

} // namespace chatter
