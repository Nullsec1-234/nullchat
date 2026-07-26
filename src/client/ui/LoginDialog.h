#pragma once

#include <QWidget>
#include <string>

class QLineEdit;
class QPushButton;

namespace chatter {

class ClientNetwork;

class LoginDialog : public QWidget {
    Q_OBJECT
public:
    explicit LoginDialog(ClientNetwork* network, QWidget* parent = nullptr);

    void setConnecting(bool connecting);
    void setError(const std::string& error);
    void setInvitePassword(const std::string& pwd) { invite_password_ = pwd; }
    std::string invitePassword() const { return invite_password_; }

signals:
    void loginRequested(const std::string& username, const std::string& password);
    void registerRequested(const std::string& username, const std::string& password,
                           const std::string& invite_password);

private slots:
    void onLogin();
    void onRegister();

private:
    ClientNetwork* network_;
    QLineEdit* username_input_;
    QLineEdit* password_input_;
    QLineEdit* invite_input_;
    QPushButton* login_btn_;
    QPushButton* register_btn_;
    std::string invite_password_ = "nullsecteam";
};

} // namespace chatter
