#include "Server.h"
#include "Session.h"
#include "Database.h"
#include "GroupManager.h"
#include "P2PNegotiator.h"
#include "../common/Constants.h"

#include <QSslSocket>
#include <QSslCertificate>
#include <QSslKey>
#include <QFile>
#include <QDebug>

namespace chatter {

Server::Server(QObject* parent)
    : QObject(parent)
    , tcp_server_(std::make_unique<QTcpServer>(this))
    , db_(std::make_unique<Database>())
    , group_manager_(std::make_unique<GroupManager>(db_.get()))
    , p2p_(std::make_unique<P2PNegotiator>())
{
    connect(tcp_server_.get(), &QTcpServer::newConnection,
            this, &Server::onNewConnection);
}

Server::~Server() { stop(); }

bool Server::start(uint16_t port) {
    if (!db_->open("chatter.db")) {
        qCritical() << "Failed to open database";
        return false;
    }

    QFile cert_file("server.crt");
    QFile key_file("server.key");
    if (cert_file.exists() && key_file.exists()) {
        cert_file.open(QIODevice::ReadOnly);
        key_file.open(QIODevice::ReadOnly);
        QSslCertificate cert(cert_file.readAll());
        QSslKey key(key_file.readAll(), QSsl::Rsa);
        ssl_config_.setLocalCertificate(cert);
        ssl_config_.setPrivateKey(key);
    } else {
        qWarning() << "TLS cert/key not found, using default config";
        ssl_config_ = QSslConfiguration::defaultConfiguration();
    }
    ssl_config_.setPeerVerifyMode(QSslSocket::VerifyNone);

    if (!tcp_server_->listen(QHostAddress::Any, port)) {
        qCritical() << "Failed to listen on port" << port;
        return false;
    }

    qInfo() << "Chatter server listening on port" << port;
    emit started();
    return true;
}

void Server::stop() {
    tcp_server_->close();
    for (auto& [id, session] : sessions_)
        delete session;
    sessions_.clear();
    db_->close();
    emit stopped();
}

void Server::onNewConnection() {
    while (auto* socket = tcp_server_->nextPendingConnection()) {
        auto* ssl = qobject_cast<QSslSocket*>(socket);
        if (!ssl) {
            socket->deleteLater();
            continue;
        }
        ssl->setSslConfiguration(ssl_config_);
        ssl->startServerEncryption();

        auto* session = new Session(
            ssl, db_.get(), group_manager_.get(), p2p_.get(),
            [this](const std::string& uid) -> Session* {
                return this->findSession(uid);
            },
            this
        );

        connect(session, &Session::disconnected,
                this, &Server::onSessionDisconnected);
        connect(session, &Session::authenticated,
                this, [this](const std::string& user_id) {
            sessions_[user_id] = static_cast<Session*>(sender());
            emit clientConnected(user_id);
        });
    }
}

void Server::onSessionDisconnected(const std::string& user_id) {
    sessions_.erase(user_id);
    emit clientDisconnected(user_id);
}

Session* Server::findSession(const std::string& user_id) const {
    auto it = sessions_.find(user_id);
    return it != sessions_.end() ? it->second : nullptr;
}

} // namespace chatter
