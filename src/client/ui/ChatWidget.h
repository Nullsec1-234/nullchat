#pragma once

#include <QWidget>
#include <string>

class QTextBrowser;

namespace chatter {

struct ChatMessage;

class ChatWidget : public QWidget {
    Q_OBJECT
public:
    explicit ChatWidget(QWidget* parent = nullptr);

    void addMessage(const ChatMessage& msg);
    void addSystemMessage(const QString& text);
    void setChatTitle(const QString& title);
    void clear();

private:
    QTextBrowser* browser_;
};

} // namespace chatter
