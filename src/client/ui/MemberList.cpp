#include "MemberList.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QFrame>

namespace chatter {

MemberList::MemberList(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(200);
    setStyleSheet("background: #0a0a0a; color: #33ff33; border-left: 1px solid #1a1a1a;");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(16, 16, 16, 16);
    layout_->setAlignment(Qt::AlignTop);

    header_ = new QLabel("> online (0)", this);
    header_->setStyleSheet("font-weight: bold; opacity: 0.7; font-size: 11px;"
                           "margin-bottom: 8px; color: #33ff33;");
    layout_->addWidget(header_);
}

void MemberList::addMember(const std::string& id, const std::string& name,
                           bool online, bool is_null) {
    auto it = members_.find(id);
    if (it != members_.end()) {
        setOnline(id, online);
        return;
    }

    auto* row = new QWidget(this);
    row->setStyleSheet("background: transparent;");
    auto* row_layout = new QHBoxLayout(row);
    row_layout->setContentsMargins(0, 2, 0, 2);
    row_layout->setSpacing(8);

    auto* dot = new QLabel(row);
    dot->setFixedSize(6, 6);
    dot->setStyleSheet(QString("background: %1; border: none;")
        .arg(online ? "#33ff33" : "#1a1a1a"));

    auto* label = new QLabel(QString::fromStdString(name), row);
    if (is_null)
        label->setStyleSheet("color: #33ff33; font-weight: bold;");
    else
        label->setStyleSheet("color: #33ff33;");

    row_layout->addWidget(dot);
    row_layout->addWidget(label);
    row_layout->addStretch();

    layout_->addWidget(row);
    members_[id] = {id, name, online, is_null, row, dot};
    updateCount();
}

void MemberList::setOnline(const std::string& id, bool online) {
    auto it = members_.find(id);
    if (it == members_.end()) return;
    it->second.online = online;
    if (it->second.dot)
        it->second.dot->setStyleSheet(QString("background: %1; border: none;")
            .arg(online ? "#33ff33" : "#1a1a1a"));
    updateCount();
}

void MemberList::clear() {
    members_.clear();
    QLayoutItem* item;
    while ((item = layout_->takeAt(1)) != nullptr) {
        delete item->widget();
        delete item;
    }
    updateCount();
}

void MemberList::updateCount() {
    int count = 0;
    for (const auto& [id, m] : members_)
        if (m.online) count++;
    header_->setText(QString("> online (%1)").arg(count));
}

} // namespace chatter
