#pragma once

#include <QWidget>
#include <string>

class QVBoxLayout;
class QPushButton;

namespace chatter {

class ServerBar : public QWidget {
    Q_OBJECT
public:
    explicit ServerBar(QWidget* parent = nullptr);

signals:
    void serverSelected(const std::string& server_id);

private:
    QVBoxLayout* layout_;
};

} // namespace chatter
