#pragma once

#include <QWidget>
#include <string>
#include <unordered_map>

class QListWidget;
class QListWidgetItem;

namespace chatter {

class ContactList : public QWidget {
    Q_OBJECT
public:
    explicit ContactList(QWidget* parent = nullptr);

    void addOrUpdateContact(const std::string& id, const std::string& name, bool online);
    void setOnline(const std::string& id, bool online);
    void updateLastMessage(const std::string& id, const std::string& content, int64_t timestamp);
    void incrementUnread(const std::string& id);
    void clearUnread(const std::string& id);
    std::string contactName(const std::string& id) const;
    void selectFirst();

    void setAdminMode(bool admin) { is_admin_ = admin; }

signals:
    void contactSelected(const std::string& id);
    void renameRequested(const std::string& id);
    void deleteRequested(const std::string& id);
    void createChannelRequested();

private slots:
    void onItemClicked(QListWidgetItem* item);
    void onContextMenu(const QPoint& pos);

private:
    QListWidget* list_;
    struct ContactInfo {
        std::string id;
        std::string name;
        std::string last_message;
        int unread = 0;
    };
    std::unordered_map<std::string, ContactInfo> contacts_;
    bool is_admin_ = false;
};

} // namespace chatter
