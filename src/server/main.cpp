#include "Server.h"
#include "../common/Constants.h"

#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    app.setApplicationName(QString::fromStdString(std::string(chatter::APP_NAME) + "-server"));

    chatter::Server server;
    if (!server.start(chatter::DEFAULT_PORT))
        return 1;

    return app.exec();
}
