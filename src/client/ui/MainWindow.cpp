#include "MainWindow.h"
#include "ASCIISplash.h"
#include "LoginDialog.h"
#include "ChatWidget.h"
#include "ContactList.h"
#include "ServerBar.h"
#include "MemberList.h"
#include "../network/ClientNetwork.h"
#include "../network/P2PManager.h"
#include "../crypto/ClientCrypto.h"
#include "../../common/Message.h"
#include "../store/MessageStore.h"

#include <QStackedWidget>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QInputDialog>
#include <QMessageBox>
#include <QLabel>
#include <QListWidget>
#include <QKeyEvent>
#include <QFrame>
#include <QApplication>
#include <QInputDialog>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

bool EnterFilter::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::KeyPress) {
        auto* key = static_cast<QKeyEvent*>(event);
        if (key->key() == Qt::Key_Return && !(key->modifiers() & Qt::ShiftModifier)) {
            emit enterPressed();
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

namespace chatter {

static QStringList configPaths() {
    auto portable = QDir(QCoreApplication::applicationDirPath()).filePath("nullchat.json");
    auto xdg = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + "/nullchat.json";
    auto home = QDir::homePath() + "/.nullchat.json";
    return {xdg, home, portable};
}

static QJsonObject loadConfig() {
    for (const auto& p : configPaths()) {
        QFile f(p);
        if (f.open(QIODevice::ReadOnly))
            return QJsonDocument::fromJson(f.readAll()).object();
    }
    return {};
}

static void saveConfig(const QJsonObject& cfg) {
    auto paths = configPaths();
    QFile f(paths.first());
    if (f.open(QIODevice::WriteOnly))
        f.write(QJsonDocument(cfg).toJson());
}

static QString serverHost() {
    auto cfg = loadConfig();
    return cfg.value("server").toString("38.134.182.152");
}

static uint16_t serverPort() {
    auto cfg = loadConfig();
    return static_cast<uint16_t>(cfg.value("port").toInt(8447));
}

static const char* APP_STYLESHEET = R"(
QMainWindow, QWidget {
  background: #0a0a0a;
  color: #33ff33;
  font-family: 'JetBrains Mono', 'Liberation Mono', monospace;
  font-size: 13px;
}
QTextEdit {
  background: #0d0d0d;
  color: #33ff33;
  border: 1px solid #1a1a1a;
  padding: 10px;
  font-family: 'JetBrains Mono', 'Liberation Mono', monospace;
  font-size: 13px;
}
QTextEdit:focus {
  border-color: #33ff33;
}
QPushButton {
  background: #0a0a0a;
  color: #33ff33;
  border: 1px solid #33ff33;
  padding: 8px 16px;
  font-family: 'JetBrains Mono', 'Liberation Mono', monospace;
  font-size: 12px;
}
QPushButton:hover { background: #1a1a1a; }
QPushButton:pressed { background: #2a2a2a; }
QListWidget {
  background: #0a0a0a;
  color: #33ff33;
  border: none;
  outline: none;
  font-family: 'JetBrains Mono', 'Liberation Mono', monospace;
  font-size: 13px;
}
QListWidget::item { padding: 8px 12px; }
QListWidget::item:hover { background: #111111; }
QListWidget::item:selected { background: #1a1a1a; color: #33ff33; }
QScrollBar:vertical {
  background: #0a0a0a;
  width: 6px;
}
QScrollBar::handle:vertical {
  background: #1a1a1a;
  min-height: 30px;
}
QScrollBar::add-line, QScrollBar::sub-line { height: 0; }
QScrollBar:horizontal { height: 0; }
)";

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , network_(std::make_unique<ClientNetwork>())
    , crypto_(std::make_unique<ClientCrypto>())
    , store_(std::make_unique<MessageStore>())
{
    setStyleSheet(APP_STYLESHEET);
    resize(1280, 720);
    setMinimumSize(900, 600);

    stack_ = new QStackedWidget(this);
    setCentralWidget(stack_);

    buildSplash();

    connect(network_.get(), &ClientNetwork::authSuccess, this, &MainWindow::onAuthSuccess);
    connect(network_.get(), &ClientNetwork::authError, this, &MainWindow::onAuthError);
    connect(network_.get(), &ClientNetwork::messageReceived, this, &MainWindow::onMessageReceived);
    connect(network_.get(), &ClientNetwork::groupMessageReceived, this, &MainWindow::onGroupMessageReceived);
    connect(network_.get(), &ClientNetwork::userOnline, this, &MainWindow::onUserOnline);
    connect(network_.get(), &ClientNetwork::userOffline, this, &MainWindow::onUserOffline);
    connect(network_.get(), &ClientNetwork::mentionReceived, this, [this](
        const std::string& sender_id, const std::string& sender_name,
        const std::string& group_id, const std::string& group_name,
        const std::string& content) {
        Q_UNUSED(sender_id);
        chat_widget_->addSystemMessage(
            QString("@%1 mentioned you in #%2").arg(
                QString::fromStdString(sender_name),
                QString::fromStdString(group_name)));
        QApplication::alert(this, 3000);
    });

    splash_->start();
}

MainWindow::~MainWindow() = default;

void MainWindow::buildSplash() {
    splash_ = new ASCIISplash(this);
    stack_->addWidget(splash_);
    connect(splash_, &ASCIISplash::finished, this, &MainWindow::onSplashFinished);
    stack_->setCurrentWidget(splash_);
}

void MainWindow::onSplashFinished() {
    splash_->setStatus("connecting...");
    splash_->hide();

    // Check for saved session
    auto saved = store_->loadBlob("session");
    auto saved_pin_hash = store_->loadBlob("pin_hash");
    if (!saved.empty() && !saved_pin_hash.empty()) {
        while (true) {
            bool ok;
            auto pin = QInputDialog::getText(this, "unlock", "enter PIN:",
                                              QLineEdit::Password, "", &ok);
            if (!ok) break;
            if (pin.isEmpty()) continue;
            auto pin_hash = crypto_->sha256(pin.toStdString());
            if (pin_hash != saved_pin_hash) {
                QMessageBox::warning(this, "error", "wrong PIN");
                continue;
            }
            auto sep = saved.find(':');
            if (sep != std::string::npos) {
                auto user = saved.substr(0, sep);
                auto pass = saved.substr(sep + 1);
                splash_->setStatus(QString("auto-login as %1...").arg(QString::fromStdString(user)));
                network_->setPublicKey(crypto_->getECPublicKey());
                if (network_->connectAndLogin(serverHost().toStdString(), serverPort(), user, pass,
                                               crypto_->getECPublicKey()))
                    return;
            }
        }
    }

    login_dialog_ = new LoginDialog(network_.get(), this);
    login_dialog_->setStyleSheet("background: #0a0a0a; color: #33ff33;");
    stack_->addWidget(login_dialog_);
    stack_->setCurrentWidget(login_dialog_);

    connect(login_dialog_, &LoginDialog::loginRequested,
            this, &MainWindow::onLoginRequested);
    connect(login_dialog_, &LoginDialog::registerRequested,
            this, &MainWindow::onRegisterRequested);
}

void MainWindow::buildApp() {
    app_page_ = new QWidget(this);
    auto* main_layout = new QHBoxLayout(app_page_);
    main_layout->setContentsMargins(0, 0, 0, 0);
    main_layout->setSpacing(0);

    // Server bar (leftmost)
    server_bar_ = new ServerBar(app_page_);
    server_bar_->setStyleSheet(
        "ServerBar { background: #0a0a0a; border-right: 1px solid #1a1a1a; }");

    // Channel list
    auto* channel_container = new QWidget(app_page_);
    channel_container->setFixedWidth(250);
    channel_container->setStyleSheet(
        "border-right: 1px solid #1a1a1a;");

    auto* channel_layout = new QVBoxLayout(channel_container);
    channel_layout->setContentsMargins(0, 0, 0, 0);
    channel_layout->setSpacing(0);

    auto* server_title = new QLabel("  # nullsec", channel_container);
    server_title->setFixedHeight(48);
    server_title->setStyleSheet(
        "font-weight: bold; font-size: 14px; padding-left: 12px; color: #33ff33;"
        "border-bottom: 1px solid #1a1a1a; background: #0a0a0a;");

    channel_list_ = new ContactList(channel_container);
    channel_list_->setStyleSheet(
        "ContactList { background: #0a0a0a; }"
        "QListWidget { background: #0a0a0a; color: #33ff33; border: none;"
        "  font-size: 13px; }"
        "QListWidget::item { padding: 8px 12px; }"
        "QListWidget::item:hover { background: #111111; }");

    channel_layout->addWidget(server_title);
    channel_layout->addWidget(channel_list_, 1);

    // Chat area (center)
    auto* chat_container = new QWidget(app_page_);
    auto* chat_layout = new QVBoxLayout(chat_container);
    chat_layout->setContentsMargins(0, 0, 0, 0);
    chat_layout->setSpacing(0);

    chat_header_ = new QLabel("  > select a channel", chat_container);
    chat_header_->setFixedHeight(48);
    chat_header_->setStyleSheet(
        "font-weight: bold; font-size: 13px; padding-left: 16px; color: #33ff33;"
        "border-bottom: 1px solid #1a1a1a; background: #0a0a0a;");

    chat_widget_ = new ChatWidget(chat_container);
    chat_widget_->setStyleSheet("background: #0a0a0a;");

    auto* input_container = new QWidget(chat_container);
    auto* input_layout = new QHBoxLayout(input_container);
    input_layout->setContentsMargins(12, 8, 12, 12);

    message_input_ = new QTextEdit(input_container);
    message_input_->setMaximumHeight(60);
    message_input_->setPlaceholderText("> type a message...");
    message_input_->setFixedHeight(40);

    auto* filter = new EnterFilter(this);
    connect(filter, &EnterFilter::enterPressed, this, &MainWindow::onSendMessage);
    message_input_->installEventFilter(filter);

    send_btn_ = new QPushButton("send", input_container);
    send_btn_->setFixedHeight(40);

    input_layout->addWidget(message_input_);
    input_layout->addWidget(send_btn_);

    chat_layout->addWidget(chat_header_);
    chat_layout->addWidget(chat_widget_, 1);
    chat_layout->addWidget(input_container);

    // Member list (right)
    member_list_ = new MemberList(app_page_);

    // Assemble
    main_layout->addWidget(server_bar_);
    main_layout->addWidget(channel_container);
    main_layout->addWidget(chat_container, 1);
    main_layout->addWidget(member_list_);

    connect(send_btn_, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(channel_list_, &ContactList::renameRequested, this, [this](const std::string& id) {
        bool ok;
        auto name = QInputDialog::getText(this, "rename channel", "new name:",
                                           QLineEdit::Normal,
                                           QString::fromStdString(channel_list_->contactName(id)), &ok);
        if (ok && !name.isEmpty())
            network_->renameGroup(id, name.toStdString());
    });
    connect(channel_list_, &ContactList::deleteRequested, this, [this](const std::string& id) {
        if (QMessageBox::question(this, "delete channel", "delete this channel?",
                                   QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
            network_->deleteGroup(id);
    });
    connect(channel_list_, &ContactList::createChannelRequested, this, [this]() {
        bool ok;
        auto name = QInputDialog::getText(this, "create channel", "channel name:",
                                           QLineEdit::Normal, "", &ok);
        if (ok && !name.isEmpty())
            network_->createGroup(name.toStdString(), "");
    });
    connect(channel_list_, &ContactList::contactSelected, this, [this](const std::string& id) {
        current_channel_id_ = id;
        channel_list_->clearUnread(id);
        auto name = channel_list_->contactName(id);
        chat_header_->setText(QString("  > #%1").arg(QString::fromStdString(name)));
        message_input_->setPlaceholderText(QString("> message #%1...").arg(QString::fromStdString(name)));
        chat_widget_->clear();
        for (const auto& m : store_->loadMessages(id, 100))
            chat_widget_->addMessage(m);
    });

    stack_->addWidget(app_page_);
}

void MainWindow::switchToApp() {
    buildApp();

    for (const auto& g : network_->initialGroups())
        channel_list_->addOrUpdateContact(g.id, g.name, true);
    channel_list_->selectFirst();
    channel_list_->setAdminMode(is_null_);
    member_list_->addMember(user_id_, username_, true, is_null_);

    stack_->setCurrentWidget(app_page_);
    setWindowTitle(QString("nullchat - %1%2")
        .arg(QString::fromStdString(username_))
        .arg(is_null_ ? " [admin]" : ""));
}

void MainWindow::onLoginRequested(const std::string& username, const std::string& password) {
    login_dialog_->setConnecting(true);
    splash_->setStatus(QString("logging in as %1...").arg(QString::fromStdString(username)));
    last_password_ = password;
    network_->setPublicKey(crypto_->getECPublicKey());
    network_->connectAndLogin(serverHost().toStdString(), serverPort(), username, password, crypto_->getECPublicKey());
}

void MainWindow::onRegisterRequested(const std::string& username, const std::string& password,
                                      const std::string& invite_password) {
    login_dialog_->setConnecting(true);
    splash_->setStatus(QString("registering %1...").arg(QString::fromStdString(username)));
    last_password_ = password;
    network_->setPublicKey(crypto_->getECPublicKey());
    network_->connectAndRegister(serverHost().toStdString(), serverPort(), username, password,
                                 crypto_->getECPublicKey(), invite_password);
}

void MainWindow::onAuthSuccess(const std::string& user_id, const std::string& username) {
    user_id_ = user_id;
    username_ = username;
    is_null_ = network_->isNull();

    p2p_ = std::make_unique<P2PManager>(crypto_.get());
    connect(network_.get(), &ClientNetwork::p2pOfferReceived,
            p2p_.get(), &P2PManager::handleOffer);

    // Prompt to set PIN if no session saved yet
    if (store_->loadBlob("session").empty() && !last_password_.empty()) {
        bool ok;
        auto pin = QInputDialog::getText(this, "auto-login", "set a PIN for auto-login:",
                                          QLineEdit::Password, "", &ok);
        if (ok && !pin.isEmpty()) {
            store_->saveBlob("pin_hash", crypto_->sha256(pin.toStdString()));
            store_->saveBlob("session", username + ":" + last_password_);
        }
    }

    switchToApp();
}

void MainWindow::onAuthError(const std::string& error) {
    if (login_dialog_) {
        login_dialog_->setConnecting(false);
        splash_->setStatus(QString::fromStdString(error));
        QMessageBox::warning(login_dialog_, "error", QString::fromStdString(error));
    } else {
        splash_->setStatus(QString("auth failed: %1").arg(QString::fromStdString(error)));
    }
}

void MainWindow::onMessageReceived(const ChatMessage& msg) {
    auto m = msg;
    if (m.id.empty())
        m.id = std::to_string(m.timestamp) + "_" + m.sender_id;
    store_->saveMessage(m);
    if (m.group_id.empty() && m.sender_id != user_id_) {
        channel_list_->addOrUpdateContact(m.sender_id, m.sender_name, true);
        if (m.sender_id != current_channel_id_)
            channel_list_->incrementUnread(m.sender_id);
    }
    member_list_->addMember(m.sender_id, m.sender_name, true, false);
}

void MainWindow::onGroupMessageReceived(const ChatMessage& msg) {
    auto m = msg;
    if (m.id.empty())
        m.id = std::to_string(m.timestamp) + "_" + m.sender_id;
    if (m.group_id == current_channel_id_)
        chat_widget_->addMessage(m);
    else
        channel_list_->incrementUnread(m.group_id);
    store_->saveMessage(m);
    if (m.sender_id != user_id_)
        member_list_->addMember(m.sender_id, m.sender_name, true, false);
}

void MainWindow::onUserOnline(const std::string& user_id, const std::string& username) {
    member_list_->addMember(user_id, username, true, false);
}

void MainWindow::onUserOffline(const std::string& user_id) {
    member_list_->setOnline(user_id, false);
}

void MainWindow::onSendMessage() {
    auto text = message_input_->toPlainText().trimmed();
    if (text.isEmpty() || current_channel_id_.empty())
        return;
    message_input_->clear();

    ChatMessage msg;
    msg.id = std::to_string(msg.now()) + "_" + user_id_;
    msg.sender_id = user_id_;
    msg.sender_name = username_;
    msg.content = text.toStdString();
    msg.group_id = current_channel_id_;
    msg.timestamp = msg.now();

    network_->sendGroupMessage(msg);
    chat_widget_->addMessage(msg);
    store_->saveMessage(msg);
}

void MainWindow::onChannelSelected(QListWidgetItem* item) {
    Q_UNUSED(item);
}

void MainWindow::onCreateGroup() {
    bool ok;
    auto name = QInputDialog::getText(this, "new group", "group name:",
                                       QLineEdit::Normal, "", &ok);
    if (!ok || name.isEmpty()) return;
    network_->createGroup(name.toStdString(), "");
}

} // namespace chatter
