#pragma once

#include <QWidget>
#include <string>
#include <unordered_map>

class QVBoxLayout;
class QLabel;

namespace chatter {

class MemberList : public QWidget {
    Q_OBJECT
public:
    explicit MemberList(QWidget* parent = nullptr);

    void addMember(const std::string& id, const std::string& name, bool online, bool is_null);
    void setOnline(const std::string& id, bool online);
    void clear();
    void updateCount();

private:
    struct MemberInfo {
        std::string id;
        std::string name;
        bool online = false;
        bool is_null = false;
        QWidget* widget = nullptr;
    };

    QVBoxLayout* layout_;
    QLabel* header_;
    std::unordered_map<std::string, MemberInfo> members_;
};

} // namespace chatter
