#include <QApplication>
#include <QStyleFactory>
#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName(QStringLiteral("Wols Password Generator"));
    app.setApplicationDisplayName(QStringLiteral("Wols Password Generator & Vault"));
    app.setApplicationVersion(QStringLiteral("2.0.0"));
    app.setOrganizationName(QStringLiteral("Wols"));

    if (QStyleFactory::keys().contains(QStringLiteral("Fusion"))) {
        app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));
    }

    gui::MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
