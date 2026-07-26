#pragma once

#include <string>
#include <vector>

class QSqlDatabase;

namespace chatter {

struct ChatMessage;

class MessageStore {
public:
    MessageStore();
    ~MessageStore();

    void saveMessage(const ChatMessage& msg);
    std::vector<ChatMessage> loadMessages(const std::string& group_id, int limit = 100);
    void clearGroup(const std::string& group_id);

    void saveBlob(const std::string& key, const std::string& value);
    std::string loadBlob(const std::string& key);

private:
    QSqlDatabase* db_ = nullptr;
    void ensureTable();
};

} // namespace chatter
