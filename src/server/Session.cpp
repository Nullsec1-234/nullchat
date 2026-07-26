#include "Session.h"
#include "Database.h"
#include "GroupManager.h"
#include "P2PNegotiator.h"
#include "../common/Protocol.h"
#include "../common/Constants.h"

#include <QSslSocket>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QDebug>

namespace chatter {

Session::Session(QSslSocket* socket, Database* db,
                 GroupManager* gm, P2PNegotiator* p2p,
                 SessionFinder find_session,
                 QObject* parent)
    : QObject(parent)
    , socket_(socket)
    , db_(db)
    , gm_(gm)
    , p2p_(p2p)
    , find_session_(std::move(find_session))
{
    connect(socket_, &QSslSocket::readyRead, this, &Session::onReadyRead);
    connect(socket_, &QSslSocket::disconnected, this, &Session::onDisconnected);
}

Session::~Session() {
    if (socket_)
        socket_->deleteLater();
}

void Session::sendPacket(uint8_t type, const std::string& body) {
    auto pkt = make_packet(static_cast<MessageType>(type), body);
    auto data = pkt.serialize();
    socket_->write(reinterpret_cast<const char*>(data.data()), data.size());
    socket_->flush();
}

void Session::sendRaw(const std::vector<uint8_t>& data) {
    socket_->write(reinterpret_cast<const char*>(data.data()), data.size());
    socket_->flush();
}

void Session::onReadyRead() {
    read_buffer_ += socket_->readAll();
    while (static_cast<size_t>(read_buffer_.size()) >= sizeof(PacketHeader)) {
        PacketHeader hdr;
        std::memcpy(&hdr, read_buffer_.constData(), sizeof(PacketHeader));
        if (hdr.version != PROTOCOL_VERSION) {
            qWarning() << "Protocol version mismatch";
            socket_->disconnectFromHost();
            return;
        }
        size_t total = sizeof(PacketHeader) + hdr.length;
        if (static_cast<size_t>(read_buffer_.size()) < total)
            return;

        std::string body(read_buffer_.constData() + sizeof(PacketHeader), hdr.length);
        read_buffer_.remove(0, static_cast<qsizetype>(total));
        handlePacket(hdr.type, body);
    }
}

void Session::onDisconnected() {
    if (!user_id_.empty()) {
        QJsonObject status;
        status["user_id"] = QString::fromStdString(user_id_);
        status["username"] = QString::fromStdString(username_);
        status["online"] = false;
        auto pkt = make_packet(MessageType::UserOffline,
                               QJsonDocument(status).toJson().toStdString());
        gm_->broadcastToGroup("", user_id_, pkt, find_session_); // no-op for ""
    }
    emit disconnected(user_id_);
    if (socket_) {
        socket_->deleteLater();
        socket_ = nullptr;
    }
}

void Session::handlePacket(uint8_t type, const std::string& body) {
    switch (static_cast<MessageType>(type)) {
    case MessageType::AuthLogin:        handleAuthLogin(body); break;
    case MessageType::AuthRegister:     handleAuthRegister(body); break;
    case MessageType::TextMessage:      handleTextMessage(body); break;
    case MessageType::GroupMessage:     handleGroupMessage(body); break;
    case MessageType::CreateGroup:      handleCreateGroup(body); break;
    case MessageType::JoinGroup:        handleJoinGroup(body); break;
    case MessageType::P2POffer:         handleP2POffer(body); break;
    case MessageType::P2PAnswer:        handleP2PAnswer(body); break;
    case MessageType::P2PICECandidate:  handleP2PICECandidate(body); break;
    case MessageType::Ping: {
        auto pong = make_packet(MessageType::Pong, "{}");
        auto data = pong.serialize();
        socket_->write(reinterpret_cast<const char*>(data.data()), data.size());
        break;
    }
    default:
        qWarning() << "Unknown packet type:" << (int)type;
    }
}

void Session::handleAuthLogin(const std::string& json) {
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto username = obj["username"].toString().toStdString();
    auto password = obj["password"].toString().toStdString();

    QJsonObject resp;
    auto [ok, id] = db_->loginUser(username, password);
    if (ok) {
        authenticated_ = true;
        user_id_ = id;
        username_ = username;
        resp["status"] = "ok";
        resp["user_id"] = QString::fromStdString(id);
        resp["username"] = QString::fromStdString(username);

        auto groups = gm_->getUserGroups(id);
        QJsonArray arr;
        for (auto& g : groups) {
            arr.append(QJsonObject{
                {"id", QString::fromStdString(g.id)},
                {"name", QString::fromStdString(g.name)}
            });
        }
        resp["groups"] = arr;

        emit authenticated(user_id_);

        QJsonObject online;
        online["user_id"] = QString::fromStdString(id);
        online["username"] = QString::fromStdString(username);
        online["online"] = true;
        auto pkt = make_packet(MessageType::UserOnline,
                               QJsonDocument(online).toJson().toStdString());
        gm_->broadcastToGroup("", user_id_, pkt, find_session_);
    } else {
        resp["status"] = "error";
        resp["error"] = "Invalid credentials";
    }

    sendPacket(static_cast<uint8_t>(MessageType::AuthResponse),
               QJsonDocument(resp).toJson().toStdString());
}

void Session::handleAuthRegister(const std::string& json) {
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto username = obj["username"].toString().toStdString();
    auto password = obj["password"].toString().toStdString();

    QJsonObject resp;
    auto [ok, id] = db_->registerUser(username, password);
    if (ok)
        resp["status"] = "ok";
    else
        resp["status"] = "error";

    sendPacket(static_cast<uint8_t>(MessageType::AuthResponse),
               QJsonDocument(resp).toJson().toStdString());
}

void Session::handleTextMessage(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto recipient_id = obj["recipient_id"].toString().toStdString();

    QJsonObject fwd;
    fwd["sender_id"] = QString::fromStdString(user_id_);
    fwd["sender_name"] = QString::fromStdString(username_);
    fwd["content"] = obj["content"];
    fwd["encrypted"] = obj["encrypted"];
    fwd["timestamp"] = obj["timestamp"];

    if (auto* session = find_session_(recipient_id))
        session->sendPacket(static_cast<uint8_t>(MessageType::TextMessage),
                            QJsonDocument(fwd).toJson().toStdString());
}

void Session::handleGroupMessage(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto group_id = obj["group_id"].toString().toStdString();

    QJsonObject fwd;
    fwd["sender_id"] = QString::fromStdString(user_id_);
    fwd["sender_name"] = QString::fromStdString(username_);
    fwd["content"] = obj["content"];
    fwd["encrypted"] = obj["encrypted"];
    fwd["timestamp"] = obj["timestamp"];
    fwd["group_id"] = QString::fromStdString(group_id);

    auto pkt = make_packet(MessageType::GroupMessage,
                           QJsonDocument(fwd).toJson().toStdString());
    gm_->broadcastToGroup(group_id, user_id_, pkt, find_session_);
}

void Session::handleCreateGroup(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto name = obj["name"].toString().toStdString();
    auto encrypted_key = obj["encrypted_key"].toString().toStdString();

    auto group_id = gm_->createGroup(name, user_id_, encrypted_key);
    QJsonObject resp;
    resp["status"] = group_id.empty() ? "error" : "ok";
    resp["group_id"] = QString::fromStdString(group_id);
    resp["name"] = QString::fromStdString(name);
    sendPacket(static_cast<uint8_t>(MessageType::GroupInfo),
               QJsonDocument(resp).toJson().toStdString());
}

void Session::handleJoinGroup(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto group_id = obj["group_id"].toString().toStdString();
    auto encrypted_key = obj["encrypted_key"].toString().toStdString();

    gm_->addMember(group_id, user_id_, encrypted_key);
    QJsonObject resp;
    resp["status"] = "ok";
    resp["group_id"] = QString::fromStdString(group_id);
    sendPacket(static_cast<uint8_t>(MessageType::GroupInfo),
               QJsonDocument(resp).toJson().toStdString());
}

void Session::handleP2POffer(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto target_id = obj["target_id"].toString().toStdString();

    if (auto* target = find_session_(target_id)) {
        QJsonObject offer;
        offer["from_id"] = QString::fromStdString(user_id_);
        offer["from_username"] = QString::fromStdString(username_);
        offer["sdp"] = obj["sdp"];
        offer["public_key"] = obj["public_key"];
        target->sendPacket(static_cast<uint8_t>(MessageType::P2POffer),
                           QJsonDocument(offer).toJson().toStdString());
    }
}

void Session::handleP2PAnswer(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto target_id = obj["target_id"].toString().toStdString();

    if (auto* target = find_session_(target_id)) {
        QJsonObject answer;
        answer["from_id"] = QString::fromStdString(user_id_);
        answer["sdp"] = obj["sdp"];
        answer["public_key"] = obj["public_key"];
        target->sendPacket(static_cast<uint8_t>(MessageType::P2PAnswer),
                           QJsonDocument(answer).toJson().toStdString());
    }
}

void Session::handleP2PICECandidate(const std::string& json) {
    if (!authenticated_) return;
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(json));
    auto obj = doc.object();
    auto target_id = obj["target_id"].toString().toStdString();

    if (auto* target = find_session_(target_id)) {
        QJsonObject ice;
        ice["from_id"] = QString::fromStdString(user_id_);
        ice["candidate"] = obj["candidate"];
        target->sendPacket(static_cast<uint8_t>(MessageType::P2PICECandidate),
                           QJsonDocument(ice).toJson().toStdString());
    }
}

} // namespace chatter
