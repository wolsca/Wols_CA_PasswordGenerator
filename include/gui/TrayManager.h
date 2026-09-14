#pragma once

#include <QObject>
#include <QSystemTrayIcon>
#include <QMenu>

namespace gui {

class MainWindow;

class TrayManager : public QObject {
    Q_OBJECT
public:
    explicit TrayManager(MainWindow* mainWindow);

    void showNotification(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon = QSystemTrayIcon::Information);

private slots:
    void onTrayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void onQuickGenerateAndCopy();

private:
    void setupTray();

    MainWindow* m_mainWindow = nullptr;
    QSystemTrayIcon* m_trayIcon = nullptr;
    QMenu* m_trayMenu = nullptr;
};

} // namespace gui
