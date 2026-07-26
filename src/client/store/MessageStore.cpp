#include "MessageStore.h"
#include "../../common/Message.h"

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QCoreApplication>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

namespace chatter {

MessageStore::MessageStore() {
    auto dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    auto path = dir + "/nullchat.db";
    auto* db = new QSqlDatabase(QSqlDatabase::addDatabase("QSQLITE", "msg_store"));
    db->setDatabaseName(path);
    if (!db->open()) {
        qWarning() << "MessageStore: failed to open database" << db->lastError().text();
        return;
    }
    db_ = db;
    ensureTable();
}

MessageStore::~MessageStore() {
    if (db_) {
        db_->close();
        delete db_;
    }
}

void MessageStore::ensureTable() {
    QSqlQuery q(*db_);
    q.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id TEXT PRIMARY KEY,"
        "  sender_id TEXT NOT NULL,"
        "  sender_name TEXT NOT NULL,"
        "  content TEXT NOT NULL,"
        "  group_id TEXT NOT NULL DEFAULT '',"
        "  encrypted INTEGER NOT NULL DEFAULT 0,"
        "  timestamp INTEGER NOT NULL"
        ")"
    );
    q.exec("CREATE INDEX IF NOT EXISTS idx_group ON messages(group_id, timestamp)");
    q.exec(
        "CREATE TABLE IF NOT EXISTS settings ("
        "  key TEXT PRIMARY KEY,"
        "  value TEXT NOT NULL"
        ")"
    );
}

void MessageStore::saveMessage(const ChatMessage& msg) {
    if (!db_) return;
    QSqlQuery q(*db_);
    q.prepare(
        "INSERT OR REPLACE INTO messages "
        "(id, sender_id, sender_name, content, group_id, encrypted, timestamp) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)"
    );
    q.addBindValue(QString::fromStdString(msg.id));
    q.addBindValue(QString::fromStdString(msg.sender_id));
    q.addBindValue(QString::fromStdString(msg.sender_name));
    q.addBindValue(QString::fromStdString(msg.content));
    q.addBindValue(QString::fromStdString(msg.group_id));
    q.addBindValue(msg.encrypted ? 1 : 0);
    q.addBindValue(static_cast<qint64>(msg.timestamp));
    if (!q.exec())
        qWarning() << "MessageStore: save failed" << q.lastError().text();
}

std::vector<ChatMessage> MessageStore::loadMessages(const std::string& group_id, int limit) {
    std::vector<ChatMessage> result;
    if (!db_) return result;
    QSqlQuery q(*db_);
    q.prepare(
        "SELECT id, sender_id, sender_name, content, group_id, encrypted, timestamp "
        "FROM messages WHERE group_id = ? ORDER BY timestamp ASC LIMIT ?"
    );
    q.addBindValue(QString::fromStdString(group_id));
    q.addBindValue(limit);
    if (q.exec()) {
        while (q.next()) {
            ChatMessage msg;
            msg.id = q.value(0).toString().toStdString();
            msg.sender_id = q.value(1).toString().toStdString();
            msg.sender_name = q.value(2).toString().toStdString();
            msg.content = q.value(3).toString().toStdString();
            msg.group_id = q.value(4).toString().toStdString();
            msg.encrypted = q.value(5).toInt() != 0;
            msg.timestamp = q.value(6).toLongLong();
            result.push_back(msg);
        }
    }
    return result;
}

void MessageStore::clearGroup(const std::string& group_id) {
    if (!db_) return;
    QSqlQuery q(*db_);
    q.prepare("DELETE FROM messages WHERE group_id = ?");
    q.addBindValue(QString::fromStdString(group_id));
    q.exec();
}

void MessageStore::saveBlob(const std::string& key, const std::string& value) {
    if (!db_) return;
    QSqlQuery q(*db_);
    q.prepare("INSERT OR REPLACE INTO settings (key, value) VALUES (?, ?)");
    q.addBindValue(QString::fromStdString(key));
    q.addBindValue(QString::fromStdString(value));
    q.exec();
}

std::string MessageStore::loadBlob(const std::string& key) {
    if (!db_) return {};
    QSqlQuery q(*db_);
    q.prepare("SELECT value FROM settings WHERE key = ?");
    q.addBindValue(QString::fromStdString(key));
    if (q.exec() && q.next())
        return q.value(0).toString().toStdString();
    return {};
}

} // namespace chatter
