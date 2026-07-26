#include "ASCIISplash.h"

#include <QVBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QApplication>

namespace chatter {

static const char* ASCII_LOGO = R"(
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

ASCIISplash::ASCIISplash(QWidget* parent)
    : QWidget(parent)
{
    setStyleSheet("background: #0a0a0a; color: #33ff33;");

    auto* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    logo_ = new QLabel(this);
    logo_->setFont(QFont("JetBrains Mono", 9));
    logo_->setStyleSheet("color: #33ff33;");
    logo_->setText(ASCII_LOGO);

    title_ = new QLabel("nullchat", this);
    title_->setAlignment(Qt::AlignCenter);
    title_->setStyleSheet("font-size: 42px; font-weight: bold; letter-spacing: 2px;"
                          "color: #33ff33; margin-top: 20px;");

    subtitle_ = new QLabel("connecting...", this);
    subtitle_->setAlignment(Qt::AlignCenter);
    subtitle_->setStyleSheet("opacity: 0.7; font-size: 14px; margin-top: 8px; color: #33ff33;");

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    progress_->setFixedSize(300, 8);
    progress_->setTextVisible(false);
    progress_->setStyleSheet(
        "QProgressBar { background: #1a1a1a; border: none; border-radius: 4px; }"
        "QProgressBar::chunk { background: #33ff33; border-radius: 4px; }");

    layout->addStretch();
    layout->addWidget(logo_, 0, Qt::AlignCenter);
    layout->addWidget(title_, 0, Qt::AlignCenter);
    layout->addSpacing(8);
    layout->addWidget(subtitle_, 0, Qt::AlignCenter);
    layout->addSpacing(30);
    layout->addWidget(progress_, 0, Qt::AlignCenter);
    layout->addStretch();

    timer_ = new QTimer(this);
    connect(timer_, &QTimer::timeout, this, [this]() {
        step_ += 5;
        if (step_ > 100) {
            timer_->stop();
            emit finished();
            return;
        }
        progress_->setValue(step_);
    });
}

void ASCIISplash::start() {
    step_ = 0;
    progress_->setValue(0);
    timer_->start(30);
}

void ASCIISplash::setStatus(const QString& text) {
    subtitle_->setText(text);
}

} // namespace chatter
