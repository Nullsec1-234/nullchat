#pragma once

#include <QWidget>
#include <QTimer>

class QLabel;
class QProgressBar;

namespace chatter {

class ASCIISplash : public QWidget {
    Q_OBJECT
public:
    explicit ASCIISplash(QWidget* parent = nullptr);

    void start();
    void setStatus(const QString& text);

signals:
    void finished();

private:
    QLabel* logo_;
    QLabel* title_;
    QLabel* subtitle_;
    QProgressBar* progress_;
    QTimer* timer_;
    int step_ = 0;
};

} // namespace chatter
