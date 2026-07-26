#pragma once

#include <QMainWindow>
#include <string>
#include <memory>
#include <unordered_map>

class QStackedWidget;
class QTextEdit;
class QPushButton;
class QListWidget;
class QListWidgetItem;
class QLabel;

class EnterFilter : public QObject {
    Q_OBJECT
public:
    explicit EnterFilter(QObject* parent = nullptr) : QObject(parent) {}
signals:
    void enterPressed();
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;
};

namespace chatter {

class ASCIISplash;
class LoginDialog;
class ChatWidget;
class ContactList;
class ServerBar;
class MemberList;
class ClientNetwork;
class P2PManager;
class ClientCrypto;
class MessageStore;

struct ChatMessage;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onSplashFinished();
    void onLoginRequested(const std::string& username, const std::string& password);
    void onRegisterRequested(const std::string& username, const std::string& password, const std::string& invite_password);
    void onAuthSuccess(const std::string& user_id, const std::string& username);
    void onAuthError(const std::string& error);
    void onMessageReceived(const ChatMessage& msg);
    void onGroupMessageReceived(const ChatMessage& msg);
    void onUserOnline(const std::string& user_id, const std::string& username);
    void onUserOffline(const std::string& user_id);
    void onSendMessage();
    void onChannelSelected(QListWidgetItem* item);
    void onCreateGroup();

private:
    void buildSplash();
    void buildApp();
    void switchToApp();

    QStackedWidget* stack_;

    // Splash
    ASCIISplash* splash_;

    // Login
    LoginDialog* login_dialog_;

    // App
    QWidget* app_page_;
    ServerBar* server_bar_;
    ContactList* channel_list_;
    ChatWidget* chat_widget_;
    MemberList* member_list_;
    QTextEdit* message_input_;
    QPushButton* send_btn_;
    QLabel* chat_header_;

    std::unique_ptr<ClientNetwork> network_;
    std::unique_ptr<P2PManager> p2p_;
    std::unique_ptr<ClientCrypto> crypto_;
    std::unique_ptr<MessageStore> store_;

    std::string current_channel_id_;
    std::string user_id_;
    std::string username_;
    std::string last_password_;
    bool is_null_ = false;
};

} // namespace chatter
