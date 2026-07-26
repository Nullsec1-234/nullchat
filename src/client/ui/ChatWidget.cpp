#include "ChatWidget.h"
#include "../../common/Message.h"

#include <QVBoxLayout>
#include <QTextBrowser>
#include <QDateTime>
#include <QRegularExpression>

namespace chatter {

ChatWidget::ChatWidget(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    browser_ = new QTextBrowser(this);
    browser_->setOpenExternalLinks(false);
    browser_->setReadOnly(true);
    browser_->setStyleSheet(
        "QTextBrowser { background: #0a0a0a; color: #33ff33; border: none;"
        "  font-family: 'JetBrains Mono', 'Liberation Mono', monospace;"
        "  font-size: 13px; }");

    layout->addWidget(browser_);
}

void ChatWidget::addMessage(const ChatMessage& msg) {
    auto time = QDateTime::fromMSecsSinceEpoch(msg.timestamp).toString("HH:mm");
    auto sender = QString::fromStdString(msg.sender_name).toHtmlEscaped();
    auto content = QString::fromStdString(msg.content).toHtmlEscaped();

    // Highlight @mentions
    content.replace(QRegularExpression("@(\\w+)"), "<span style=\"color:#46ff7b;font-weight:bold\">@\\1</span>");

    QString html;
    if (msg.encrypted) {
        html = QString("[%1] &lt;%2&gt; <i>[encrypted]</i>").arg(time, sender);
    } else {
        html = QString("[%1] &lt;%2&gt; %3").arg(time, sender, content);
    }

    browser_->append(html);
}

void ChatWidget::addSystemMessage(const QString& text) {
    browser_->append(QString("<span style=\"color:#888888;\">[system] %1</span>").arg(text));
}

void ChatWidget::setChatTitle(const QString& title) {
    Q_UNUSED(title);
}

void ChatWidget::clear() {
    browser_->clear();
}

} // namespace chatter
