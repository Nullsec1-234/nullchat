#include "Database.h"
#include "../common/Crypto.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QUuid>

namespace chatter {

Database::Database() = default;
Database::~Database() { close(); }

bool Database::open(const std::string& path) {
    db_path_ = path;
    db_ = QSqlDatabase::addDatabase("QSQLITE", "chatter_main");
    db_.setDatabaseName(QString::fromStdString(path));
    if (!db_.open()) {
        qCritical() << "Database open failed:" << db_.lastError().text();
        return false;
    }
    return createTables();
}

void Database::close() {
    if (db_.isOpen())
        db_.close();
}

bool Database::createTables() {
    QSqlQuery q(db_);
    return q.exec(
        "CREATE TABLE IF NOT EXISTS users ("
        "  id TEXT PRIMARY KEY,"
        "  username TEXT UNIQUE NOT NULL,"
        "  password_hash TEXT NOT NULL,"
        "  public_key TEXT,"
        "  created_at INTEGER DEFAULT (strftime('%s','now'))"
        ");"
    ) && q.exec(
        "CREATE TABLE IF NOT EXISTS groups ("
        "  id TEXT PRIMARY KEY,"
        "  name TEXT NOT NULL,"
        "  owner_id TEXT NOT NULL,"
        "  created_at INTEGER DEFAULT (strftime('%s','now'))"
        ");"
    ) && q.exec(
        "CREATE TABLE IF NOT EXISTS group_members ("
        "  user_id TEXT NOT NULL,"
        "  group_id TEXT NOT NULL,"
        "  encrypted_symmetric_key TEXT,"
        "  PRIMARY KEY (user_id, group_id)"
        ");"
    ) && q.exec(
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  sender_id TEXT NOT NULL,"
        "  group_id TEXT,"
        "  content TEXT NOT NULL,"
        "  encrypted INTEGER DEFAULT 0,"
        "  timestamp INTEGER DEFAULT (strftime('%s','now'))"
        ");"
    );
}

std::tuple<bool, std::string> Database::registerUser(const std::string& username,
                                                     const std::string& password) {
    Crypto crypto;
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    auto hash = crypto.sha256(password);

    QSqlQuery q(db_);
    q.prepare("INSERT INTO users (id, username, password_hash) VALUES (?, ?, ?)");
    q.addBindValue(QString::fromStdString(id));
    q.addBindValue(QString::fromStdString(username));
    q.addBindValue(QString::fromStdString(hash));

    if (q.exec())
        return {true, id};
    return {false, ""};
}

std::tuple<bool, std::string> Database::loginUser(const std::string& username,
                                                   const std::string& password) {
    Crypto crypto;
    auto hash = crypto.sha256(password);

    QSqlQuery q(db_);
    q.prepare("SELECT id FROM users WHERE username = ? AND password_hash = ?");
    q.addBindValue(QString::fromStdString(username));
    q.addBindValue(QString::fromStdString(hash));

    if (q.exec() && q.next())
        return {true, q.value(0).toString().toStdString()};
    return {false, ""};
}

std::string Database::getPublicKey(const std::string& user_id) {
    QSqlQuery q(db_);
    q.prepare("SELECT public_key FROM users WHERE id = ?");
    q.addBindValue(QString::fromStdString(user_id));
    if (q.exec() && q.next())
        return q.value(0).toString().toStdString();
    return "";
}

std::string Database::createGroup(const std::string& name, const std::string& owner_id,
                                  const std::string& encrypted_key) {
    auto id = QUuid::createUuid().toString(QUuid::WithoutBraces).toStdString();
    QSqlQuery q(db_);
    q.prepare("INSERT INTO groups (id, name, owner_id) VALUES (?, ?, ?)");
    q.addBindValue(QString::fromStdString(id));
    q.addBindValue(QString::fromStdString(name));
    q.addBindValue(QString::fromStdString(owner_id));
    if (!q.exec()) return "";

    addMember(id, owner_id, encrypted_key);
    return id;
}

bool Database::addMember(const std::string& group_id, const std::string& user_id,
                         const std::string& encrypted_key) {
    QSqlQuery q(db_);
    q.prepare("INSERT OR REPLACE INTO group_members (user_id, group_id, encrypted_symmetric_key) "
              "VALUES (?, ?, ?)");
    q.addBindValue(QString::fromStdString(user_id));
    q.addBindValue(QString::fromStdString(group_id));
    q.addBindValue(QString::fromStdString(encrypted_key));
    return q.exec();
}

std::vector<std::string> Database::getGroupMembers(const std::string& group_id) {
    std::vector<std::string> members;
    QSqlQuery q(db_);
    q.prepare("SELECT user_id FROM group_members WHERE group_id = ?");
    q.addBindValue(QString::fromStdString(group_id));
    if (q.exec()) {
        while (q.next())
            members.push_back(q.value(0).toString().toStdString());
    }
    return members;
}

std::vector<GroupRecord> Database::getUserGroups(const std::string& user_id) {
    std::vector<GroupRecord> groups;
    QSqlQuery q(db_);
    q.prepare("SELECT g.id, g.name, g.owner_id FROM groups g "
              "JOIN group_members m ON g.id = m.group_id WHERE m.user_id = ?");
    q.addBindValue(QString::fromStdString(user_id));
    if (q.exec()) {
        while (q.next())
            groups.push_back({q.value(0).toString().toStdString(),
                              q.value(1).toString().toStdString(),
                              q.value(2).toString().toStdString()});
    }
    return groups;
}

void Database::storeMessage(const std::string& sender_id, const std::string& group_id,
                            const std::string& content, bool encrypted) {
    QSqlQuery q(db_);
    q.prepare("INSERT INTO messages (sender_id, group_id, content, encrypted) VALUES (?, ?, ?, ?)");
    q.addBindValue(QString::fromStdString(sender_id));
    q.addBindValue(QString::fromStdString(group_id));
    q.addBindValue(QString::fromStdString(content));
    q.addBindValue(encrypted ? 1 : 0);
    q.exec();
}

} // namespace chatter
