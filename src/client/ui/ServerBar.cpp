#include "ServerBar.h"

#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>

namespace chatter {

ServerBar::ServerBar(QWidget* parent)
    : QWidget(parent)
{
    setFixedWidth(80);
    setStyleSheet("ServerBar { background: #0a0a0a; border-right: 1px solid #1a1a1a; }");

    layout_ = new QVBoxLayout(this);
    layout_->setContentsMargins(14, 14, 14, 14);
    layout_->setSpacing(10);
    layout_->setAlignment(Qt::AlignTop);

    auto* null_btn = new QPushButton("N", this);
    null_btn->setFixedSize(52, 52);
    null_btn->setStyleSheet(
        "QPushButton { background: #0a0a0a; color: #33ff33; font-weight: bold;"
        "border: 1px solid #33ff33; font-size: 20px; }"
        "QPushButton:hover { background: #1a1a1a; }");
    layout_->addWidget(null_btn, 0, Qt::AlignCenter);

    auto* add_btn = new QPushButton("+", this);
    add_btn->setFixedSize(52, 52);
    add_btn->setStyleSheet(
        "QPushButton { background: #0a0a0a; color: #33ff33; font-size: 24px; font-weight: bold;"
        "border: 1px solid #1a1a1a; }"
        "QPushButton:hover { background: #1a1a1a; border-color: #33ff33; }");
    layout_->addWidget(add_btn, 0, Qt::AlignCenter);
}

} // namespace chatter
