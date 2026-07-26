#include "LoginDialog.h"
#include "../network/ClientNetwork.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QMessageBox>

namespace chatter {

static const char* LOGO_ART = R"(
;lddddol:.      lxWW0
         cxXWWWWWWWWWWWW0l. dNWWX,
       dNWWXx;        ;dXWWWWNd ;
     ,XWW0,              KWWWWX
    cWWWo              xWWNodNWN
   ;NWWd              dWWNo   'XWN
  dWWk              dNWNo      ;WWK
  0WW:            'LNWWx        NWW;
 xWWo           ;KWWx          xWWo
  XWX         '0WW0'          xWWWi
  :WW0     kNWX;             cWWWx
   ;XWNkNWXc               .xWWWO
    'NWWWWd              :kNWNKo
     oXWW0NNWWK0xddxk0WWWW0c c
     NWW0.  : ' :o0XNNNNWKo,
    cNXk              'd
       l               l
        :
                                #nullsec
)";

LoginDialog::LoginDialog(ClientNetwork* network, QWidget* parent)
    : QWidget(parent), network_(network)
{
    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    auto* logo_label = new QLabel(LOGO_ART, this);
    logo_label->setFont(QFont("JetBrains Mono", 8));
    logo_label->setStyleSheet("color: #33ff33;");

    auto* title = new QLabel("nullchat", this);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; margin-bottom: 4px; color: #33ff33;");

    auto* subtitle = new QLabel("nullsec", this);
    subtitle->setAlignment(Qt::AlignCenter);
    subtitle->setStyleSheet("font-size: 12px; opacity: 0.6; margin-bottom: 24px; color: #33ff33;");

    auto* form = new QWidget(this);
    form->setMaximumWidth(350);
    auto* form_layout = new QVBoxLayout(form);

    username_input_ = new QLineEdit(form);
    username_input_->setPlaceholderText("username");
    username_input_->setStyleSheet(
        "QLineEdit { background: #0a0a0a; color: #33ff33; border: 1px solid #1a1a1a;"
        "  padding: 12px; font-size: 14px; font-family: 'JetBrains Mono', monospace; }");

    password_input_ = new QLineEdit(form);
    password_input_->setPlaceholderText("password");
    password_input_->setEchoMode(QLineEdit::Password);
    password_input_->setStyleSheet(
        "QLineEdit { background: #0a0a0a; color: #33ff33; border: 1px solid #1a1a1a;"
        "  padding: 12px; font-size: 14px; font-family: 'JetBrains Mono', monospace; }");

    invite_input_ = new QLineEdit(form);
    invite_input_->setPlaceholderText("invite password (required)");
    invite_input_->setEchoMode(QLineEdit::Password);
    invite_input_->setStyleSheet(
        "QLineEdit { background: #0a0a0a; color: #33ff33; border: 1px solid #1a1a1a;"
        "  padding: 12px; font-size: 14px; font-family: 'JetBrains Mono', monospace; }");

    auto* btn_row = new QWidget(form);
    auto* btn_layout = new QHBoxLayout(btn_row);
    btn_layout->setContentsMargins(0, 0, 0, 0);

    login_btn_ = new QPushButton("login", btn_row);
    login_btn_->setStyleSheet(
        "QPushButton { background: #0a0a0a; color: #33ff33; border: 1px solid #33ff33;"
        "  padding: 10px; font-weight: bold; font-family: 'JetBrains Mono', monospace; }"
        "QPushButton:hover { background: #1a1a1a; }"
        "QPushButton:disabled { color: #1a5a1a; border-color: #1a5a1a; }");
    register_btn_ = new QPushButton("register", btn_row);
    register_btn_->setStyleSheet(
        "QPushButton { background: #0a0a0a; color: #33ff33; border: 1px solid #1a1a1a;"
        "  padding: 10px; font-weight: bold; font-family: 'JetBrains Mono', monospace; }"
        "QPushButton:hover { background: #1a1a1a; }"
        "QPushButton:disabled { color: #1a5a1a; border-color: #1a5a1a; }");
    btn_layout->addWidget(login_btn_);
    btn_layout->addWidget(register_btn_);

    form_layout->addWidget(username_input_);
    form_layout->addWidget(password_input_);
    form_layout->addWidget(invite_input_);
    form_layout->addSpacing(8);
    form_layout->addWidget(btn_row);

    layout->addStretch();
    layout->addWidget(logo_label, 0, Qt::AlignCenter);
    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addWidget(subtitle, 0, Qt::AlignCenter);
    layout->addWidget(form, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(login_btn_, &QPushButton::clicked, this, &LoginDialog::onLogin);
    connect(register_btn_, &QPushButton::clicked, this, &LoginDialog::onRegister);
}

void LoginDialog::setConnecting(bool connecting) {
    login_btn_->setEnabled(!connecting);
    register_btn_->setEnabled(!connecting);
    login_btn_->setText(connecting ? "connecting..." : "login");
}

void LoginDialog::setError(const std::string& error) {
    QMessageBox::warning(this, "error", QString::fromStdString(error));
}

void LoginDialog::onLogin() {
    auto username = username_input_->text().trimmed();
    auto password = password_input_->text();
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "error", "fill in all fields");
        return;
    }
    emit loginRequested(username.toStdString(), password.toStdString());
}

void LoginDialog::onRegister() {
    auto username = username_input_->text().trimmed();
    auto password = password_input_->text();
    auto invite = invite_input_->text().trimmed();
    if (username.isEmpty() || password.isEmpty() || invite.isEmpty()) {
        QMessageBox::warning(this, "error", "fill in all fields (invite password required)");
        return;
    }
    emit registerRequested(username.toStdString(), password.toStdString(), invite.toStdString());
}

} // namespace chatter
