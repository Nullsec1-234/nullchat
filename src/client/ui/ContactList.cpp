#include "ContactList.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QListWidgetItem>
#include <QMenu>
#include <QAction>

namespace chatter {

ContactList::ContactList(QWidget* parent)
    : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    list_ = new QListWidget(this);
    list_->setContextMenuPolicy(Qt::CustomContextMenu);
    layout->addWidget(list_);

    connect(list_, &QListWidget::itemClicked, this, &ContactList::onItemClicked);
    connect(list_, &QListWidget::customContextMenuRequested, this, &ContactList::onContextMenu);
}

static const char* UNREAD_BULLET = "● ";

static std::string displayName(const std::string& name, int unread) {
    return unread > 0 ? UNREAD_BULLET + name : name;
}

void ContactList::addOrUpdateContact(const std::string& id, const std::string& name, bool online) {
    auto it = contacts_.find(id);
    if (it != contacts_.end()) {
        it->second.name = name;
        for (int i = 0; i < list_->count(); ++i) {
            auto* item = list_->item(i);
            if (item->data(Qt::UserRole).toString().toStdString() == id) {
                item->setText(QString::fromStdString(displayName(name, it->second.unread)));
                return;
            }
        }
    }

    contacts_[id] = {id, name, "", 0};
    auto* item = new QListWidgetItem(QString::fromStdString(displayName(name, 0)), list_);
    item->setData(Qt::UserRole, QString::fromStdString(id));
    list_->addItem(item);
}

void ContactList::setOnline(const std::string& id, bool online) {
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        if (item->data(Qt::UserRole).toString().toStdString() == id) {
            auto text = item->text();
            if (online && !text.endsWith(" (online)"))
                item->setText(text + " (online)");
            else if (!online)
                item->setText(text.replace(" (online)", ""));
            break;
        }
    }
}

void ContactList::incrementUnread(const std::string& id) {
    auto it = contacts_.find(id);
    if (it == contacts_.end()) return;
    it->second.unread++;
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        if (item->data(Qt::UserRole).toString().toStdString() == id) {
            item->setText(QString::fromStdString(displayName(it->second.name, it->second.unread)));
            return;
        }
    }
}

void ContactList::clearUnread(const std::string& id) {
    auto it = contacts_.find(id);
    if (it == contacts_.end()) return;
    it->second.unread = 0;
    for (int i = 0; i < list_->count(); ++i) {
        auto* item = list_->item(i);
        if (item->data(Qt::UserRole).toString().toStdString() == id) {
            item->setText(QString::fromStdString(displayName(it->second.name, 0)));
            return;
        }
    }
}

void ContactList::updateLastMessage(const std::string& id, const std::string& content,
                                    int64_t timestamp) {
    auto it = contacts_.find(id);
    if (it != contacts_.end())
        it->second.last_message = content;
}

std::string ContactList::contactName(const std::string& id) const {
    auto it = contacts_.find(id);
    return it != contacts_.end() ? it->second.name : id;
}

void ContactList::selectFirst() {
    if (list_->count() > 0) {
        list_->setCurrentRow(0);
        onItemClicked(list_->item(0));
    }
}

void ContactList::onItemClicked(QListWidgetItem* item) {
    auto id = item->data(Qt::UserRole).toString().toStdString();
    emit contactSelected(id);
}

void ContactList::onContextMenu(const QPoint& pos) {
    auto* item = list_->itemAt(pos);
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background: #0a0a0a; color: #33ff33; border: 1px solid #1a1a1a; }"
        "QMenu::item { padding: 8px 16px; }"
        "QMenu::item:hover { background: #1a1a1a; }");

    if (item && is_admin_) {
        auto id = item->data(Qt::UserRole).toString().toStdString();
        auto name = item->text();

        auto* rename = menu.addAction("rename");
        connect(rename, &QAction::triggered, this, [this, id]() {
            emit renameRequested(id);
        });

        auto* del = menu.addAction("delete");
        connect(del, &QAction::triggered, this, [this, id]() {
            emit deleteRequested(id);
        });
    }

    auto* create = menu.addAction("create channel");
    connect(create, &QAction::triggered, this, [this]() {
        emit createChannelRequested();
    });

    menu.exec(list_->mapToGlobal(pos));
}

} // namespace chatter
