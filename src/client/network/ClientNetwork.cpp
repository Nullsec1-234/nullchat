#include "ClientNetwork.h"
#include "../../common/Protocol.h"
#include "../../common/Constants.h"
#include "../../common/Message.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtEndian>
#include <iostream>

namespace chatter {

ClientNetwork::ClientNetwork(QObject* parent)
    : QObject(parent)
    , socket_(new QTcpSocket(this))
{
    connect(socket_, &QTcpSocket::readyRead, this, &ClientNetwork::onReadyRead);
    connect(socket_, &QTcpSocket::connected, this, &ClientNetwork::onConnected);
    connect(socket_, &QTcpSocket::disconnected, this, &ClientNetwork::onDisconnected);
    connect(socket_, &QTcpSocket::errorOccurred, this, [this](QAbstractSocket::SocketError err) {
        std::cerr << "ClientNetwork: socket error " << err << " " << socket_->errorString().toStdString() << std::endl;
        if (!pending_username_.empty())
            emit authError(socket_->errorString().toStdString());
    });
}

ClientNetwork::~ClientNetwork() {
    disconnect();
}

void ClientNetwork::connectToServer(const std::string& host, uint16_t port) {
    if (socket_->state() != QAbstractSocket::UnconnectedState) {
        socket_->abort();
    }
    socket_->connectToHost(QString::fromStdString(host), port);
}

bool ClientNetwork::connectAndLogin(const std::string& host, uint16_t port,
                                     const std::string& username, const std::string& password,
                                     const std::string& public_key) {
    if (socket_->state() != QAbstractSocket::UnconnectedState)
        socket_->abort();

    socket_->connectToHost(QString::fromStdString(host), port);
    if (!socket_->waitForConnected(10000)) {
        emit authError("Connection failed: " + socket_->errorString().toStdString());
        return false;
    }

    QJsonObject obj;
    obj["username"] = QString::fromStdString(username);
    obj["password"] = QString::fromStdString(password);
    if (!public_key.empty())
        obj["public_key"] = QString::fromStdString(public_key);
    auto body = QJsonDocument(obj).toJson().toStdString();
    sendPacket(static_cast<uint8_t>(MessageType::AuthLogin), body);

    if (!socket_->waitForReadyRead(10000)) {
        emit authError("No response from server");
        return false;
    }

    read_buffer_ += socket_->readAll();
    if (static_cast<size_t>(read_buffer_.size()) >= sizeof(PacketHeader)) {
        PacketHeader hdr;
        std::memcpy(&hdr, read_buffer_.constData(), sizeof(PacketHeader));
        if (hdr.version == PROTOCOL_VERSION) {
            size_t total = sizeof(PacketHeader) + qFromBigEndian(hdr.length);
            if (static_cast<size_t>(read_buffer_.size()) >= total) {
                std::string rbody(read_buffer_.constData() + sizeof(PacketHeader),
                                 qFromBigEndian(hdr.length));
                read_buffer_.remove(0, static_cast<qsizetype>(total));
                auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(rbody));
                auto resp = doc.object();
                if (resp["status"].toString() == "ok") {
                    user_id_ = resp["user_id"].toString().toStdString();
                    username_ = resp["username"].toString().toStdString();
                    is_null_ = resp["is_null"].toBool();
                    authenticated_ = true;
                    initial_groups_.clear();
                    for (const auto& g : resp["groups"].toArray()) {
                        auto go = g.toObject();
                        ClientNetwork::GroupEntry entry;
                        entry.id = go["id"].toString().toStdString();
                        entry.name = go["name"].toString().toStdString();
                        initial_groups_.push_back(entry);
                    }
                    emit authSuccess(user_id_, username_);
                    return true;
                } else {
                    emit authError(resp["error"].toString().toStdString());
                    return false;
                }
            }
        }
    }

    onReadyRead();
    return authenticated_;
}

bool ClientNetwork::connectAndRegister(const std::string& host, uint16_t port,
                                        const std::string& username, const std::string& password,
                                        const std::string& public_key,
                                        const std::string& invite_password) {
    if (socket_->state() != QAbstractSocket::UnconnectedState)
        socket_->abort();

    socket_->connectToHost(QString::fromStdString(host), port);
    if (!socket_->waitForConnected(10000)) {
        emit authError("Connection failed: " + socket_->errorString().toStdString());
        return false;
    }

    QJsonObject obj;
    obj["username"] = QString::fromStdString(username);
    obj["password"] = QString::fromStdString(password);
    if (!public_key.empty())
        obj["public_key"] = QString::fromStdString(public_key);
    if (!invite_password.empty())
        obj["invite_password"] = QString::fromStdString(invite_password);
    auto body = QJsonDocument(obj).toJson().toStdString();
    sendPacket(static_cast<uint8_t>(MessageType::AuthRegister), body);

    if (!socket_->waitForReadyRead(10000)) {
        emit authError("No response from server");
        return false;
    }

    read_buffer_ += socket_->readAll();
    if (static_cast<size_t>(read_buffer_.size()) >= sizeof(PacketHeader)) {
        PacketHeader hdr;
        std::memcpy(&hdr, read_buffer_.constData(), sizeof(PacketHeader));
        if (hdr.version == PROTOCOL_VERSION) {
            size_t total = sizeof(PacketHeader) + qFromBigEndian(hdr.length);
            if (static_cast<size_t>(read_buffer_.size()) >= total) {
                std::string rbody(read_buffer_.constData() + sizeof(PacketHeader),
                                 qFromBigEndian(hdr.length));
                read_buffer_.remove(0, static_cast<qsizetype>(total));
                auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(rbody));
                auto resp = doc.object();
                auto status = resp["status"].toString();
                if (status == "ok") {
                    user_id_ = resp["user_id"].toString().toStdString();
                    username_ = resp["username"].toString().toStdString();
                    authenticated_ = true;
                    emit authSuccess(user_id_, username_);
                    return true;
                } else {
                    emit authError(resp["error"].toString().toStdString());
                    return false;
                }
            }
        }
    }

    onReadyRead();
    return authenticated_;
}

void ClientNetwork::sendPacket(uint8_t type, const std::string& body) {
    auto pkt = make_packet(static_cast<MessageType>(type), body);
    auto data = pkt.serialize();
    socket_->write(reinterpret_cast<const char*>(data.data()), data.size());
    socket_->flush();
}

void ClientNetwork::sendLogin(const std::string& username, const std::string& password) {
    pending_username_ = username;
    pending_password_ = password;
    is_login_ = true;

    if (socket_->state() == QAbstractSocket::ConnectedState)
        sendLoginOrRegister();
}

void ClientNetwork::sendRegister(const std::string& username, const std::string& password) {
    pending_username_ = username;
    pending_password_ = password;
    is_login_ = false;

    if (socket_->state() == QAbstractSocket::ConnectedState)
        sendLoginOrRegister();
}

void ClientNetwork::sendLoginOrRegister() {
    QJsonObject obj;
    obj["username"] = QString::fromStdString(pending_username_);
    obj["password"] = QString::fromStdString(pending_password_);
    if (!own_public_key_.empty())
        obj["public_key"] = QString::fromStdString(own_public_key_);
    sendPacket(static_cast<uint8_t>(is_login_ ? MessageType::AuthLogin : MessageType::AuthRegister),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::sendDirectMessage(const ChatMessage& msg) {
    QJsonObject obj;
    obj["recipient_id"] = QString::fromStdString(msg.recipient_id);
    obj["content"] = QString::fromStdString(msg.content);
    obj["encrypted"] = msg.encrypted;
    obj["timestamp"] = static_cast<qint64>(msg.timestamp);
    sendPacket(static_cast<uint8_t>(MessageType::TextMessage),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::sendGroupMessage(const ChatMessage& msg) {
    QJsonObject obj;
    obj["group_id"] = QString::fromStdString(msg.group_id);
    obj["content"] = QString::fromStdString(msg.content);
    obj["encrypted"] = msg.encrypted;
    obj["timestamp"] = static_cast<qint64>(msg.timestamp);
    sendPacket(static_cast<uint8_t>(MessageType::GroupMessage),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::createGroup(const std::string& name, const std::string& encrypted_key) {
    QJsonObject obj;
    obj["name"] = QString::fromStdString(name);
    obj["encrypted_key"] = QString::fromStdString(encrypted_key);
    sendPacket(static_cast<uint8_t>(MessageType::CreateGroup),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::joinGroup(const std::string& group_id, const std::string& encrypted_key) {
    QJsonObject obj;
    obj["group_id"] = QString::fromStdString(group_id);
    obj["encrypted_key"] = QString::fromStdString(encrypted_key);
    sendPacket(static_cast<uint8_t>(MessageType::JoinGroup),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::sendP2POffer(const std::string& target_id, const std::string& public_key) {
    QJsonObject obj;
    obj["target_id"] = QString::fromStdString(target_id);
    obj["sdp"] = "dummy_sdp";
    obj["public_key"] = QString::fromStdString(public_key);
    sendPacket(static_cast<uint8_t>(MessageType::P2POffer),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::renameGroup(const std::string& group_id, const std::string& new_name) {
    QJsonObject obj;
    obj["group_id"] = QString::fromStdString(group_id);
    obj["name"] = QString::fromStdString(new_name);
    sendPacket(static_cast<uint8_t>(MessageType::RenameGroup),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::deleteGroup(const std::string& group_id) {
    QJsonObject obj;
    obj["group_id"] = QString::fromStdString(group_id);
    sendPacket(static_cast<uint8_t>(MessageType::DeleteGroup),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::requestPublicKey(const std::string& user_id) {
    QJsonObject obj;
    obj["user_id"] = QString::fromStdString(user_id);
    sendPacket(static_cast<uint8_t>(MessageType::PubKeyRequest),
               QJsonDocument(obj).toJson().toStdString());
}

void ClientNetwork::onConnected() {
    std::cerr << "ClientNetwork: connected to server" << std::endl;
    emit connected();
    if (!pending_username_.empty()) {
        std::cerr << "ClientNetwork: sending credentials for " << pending_username_ << std::endl;
        sendLoginOrRegister();
    } else {
        std::cerr << "ClientNetwork: connected but no pending credentials" << std::endl;
    }
}

void ClientNetwork::onDisconnected() {
    authenticated_ = false;
    emit disconnected();
}

void ClientNetwork::onReadyRead() {
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

void ClientNetwork::handlePacket(uint8_t type, const std::string& body) {
    auto doc = QJsonDocument::fromJson(QByteArray::fromStdString(body));
    auto obj = doc.object();

    switch (static_cast<MessageType>(type)) {
    case MessageType::AuthResponse: {
        auto status = obj["status"].toString();
        std::cerr << "ClientNetwork: AuthResponse status = " << status.toStdString() << std::endl;
        pending_username_.clear();
        pending_password_.clear();
        if (status == "ok") {
            user_id_ = obj["user_id"].toString().toStdString();
            username_ = obj["username"].toString().toStdString();
            is_null_ = obj["is_null"].toBool();
            authenticated_ = true;
            initial_groups_.clear();
            for (const auto& g : obj["groups"].toArray()) {
                auto go = g.toObject();
                initial_groups_.push_back(
                    {go["id"].toString().toStdString(), go["name"].toString().toStdString()});
            }
            emit authSuccess(user_id_, username_);
        } else {
            emit authError(obj["error"].toString().toStdString());
        }
        break;
    }
    case MessageType::TextMessage: {
        ChatMessage msg;
        msg.sender_id = obj["sender_id"].toString().toStdString();
        msg.sender_name = obj["sender_name"].toString().toStdString();
        msg.content = obj["content"].toString().toStdString();
        msg.encrypted = obj["encrypted"].toBool();
        msg.timestamp = obj["timestamp"].toInteger();
        emit messageReceived(msg);
        break;
    }
    case MessageType::GroupMessage: {
        ChatMessage msg;
        msg.sender_id = obj["sender_id"].toString().toStdString();
        msg.sender_name = obj["sender_name"].toString().toStdString();
        msg.content = obj["content"].toString().toStdString();
        msg.group_id = obj["group_id"].toString().toStdString();
        msg.encrypted = obj["encrypted"].toBool();
        msg.timestamp = obj["timestamp"].toInteger();
        emit groupMessageReceived(msg);
        break;
    }
    case MessageType::UserOnline: {
        auto uid = obj["user_id"].toString().toStdString();
        auto uname = obj["username"].toString().toStdString();
        auto pubkey = obj["public_key"].toString().toStdString();
        if (!pubkey.empty())
            peer_keys_[uid] = pubkey;
        emit userOnline(uid, uname);
        break;
    }
    case MessageType::UserOffline:
        emit userOffline(obj["user_id"].toString().toStdString());
        break;
    case MessageType::GroupInfo:
        emit groupCreated(obj["group_id"].toString().toStdString(),
                          obj["name"].toString().toStdString());
        break;
    case MessageType::P2POffer:
        emit p2pOfferReceived(obj["from_id"].toString().toStdString(),
                              obj["sdp"].toString().toStdString(),
                              obj["public_key"].toString().toStdString());
        break;
    case MessageType::P2PAnswer:
        emit p2pAnswerReceived(obj["from_id"].toString().toStdString(),
                               obj["sdp"].toString().toStdString());
        break;
    case MessageType::P2PICECandidate:
        emit p2pICEReceived(obj["from_id"].toString().toStdString(),
                            obj["candidate"].toString().toStdString());
        break;
    case MessageType::PubKeyResponse: {
        auto uid = obj["user_id"].toString().toStdString();
        auto pubkey = obj["public_key"].toString().toStdString();
        if (!pubkey.empty())
            peer_keys_[uid] = pubkey;
        emit pubKeyReceived(uid, pubkey);
        break;
    }
    case MessageType::Mention:
        emit mentionReceived(
            obj["sender_id"].toString().toStdString(),
            obj["sender_name"].toString().toStdString(),
            obj["group_id"].toString().toStdString(),
            obj["group_name"].toString().toStdString(),
            obj["content"].toString().toStdString());
        break;
    case MessageType::Error: {
        auto err = obj["error"].toString().toStdString();
        std::cerr << "Server error: " << err << std::endl;
        emit authError(err);
        break;
    }
    default:
        break;
    }
}

} // namespace chatter
