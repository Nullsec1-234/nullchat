#pragma once

#include <QObject>
#include <QTcpServer>
#include <QSslConfiguration>
#include <memory>
#include <unordered_map>
#include <string>

class QSslSocket;

namespace chatter {

class Session;
class Database;
class GroupManager;
class P2PNegotiator;

class Server : public QObject {
    Q_OBJECT
public:
    explicit Server(QObject* parent = nullptr);
    ~Server();

    bool start(uint16_t port);
    void stop();

    Database* database() const { return db_.get(); }
    GroupManager* groupManager() const { return group_manager_.get(); }
    P2PNegotiator* p2pNegotiator() const { return p2p_.get(); }
    Session* findSession(const std::string& user_id) const;

signals:
    void started();
    void stopped();
    void clientConnected(const std::string& user_id);
    void clientDisconnected(const std::string& user_id);

private slots:
    void onNewConnection();
    void onSessionDisconnected(const std::string& user_id);

private:
    std::unique_ptr<QTcpServer> tcp_server_;
    std::unique_ptr<Database> db_;
    std::unique_ptr<GroupManager> group_manager_;
    std::unique_ptr<P2PNegotiator> p2p_;
    std::unordered_map<std::string, Session*> sessions_;
    QSslConfiguration ssl_config_;
};

} // namespace chatter
