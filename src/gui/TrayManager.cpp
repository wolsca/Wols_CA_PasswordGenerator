#include "gui/TrayManager.h"
#include "gui/MainWindow.h"
#include "gui/IconUtils.h"
#include <QApplication>

namespace gui {

TrayManager::TrayManager(MainWindow* mainWindow)
    : QObject(mainWindow), m_mainWindow(mainWindow)
{
    setupTray();
}

void TrayManager::setupTray() {
    m_trayIcon = new QSystemTrayIcon(this);
    m_trayIcon->setIcon(IconUtils::getIcon(IconType::App));
    m_trayIcon->setToolTip(QStringLiteral("Wols Password Generator & Vault"));

    m_trayMenu = new QMenu();

    QAction* actShow = m_trayMenu->addAction(IconUtils::getIcon(IconType::App), QStringLiteral("Wachtwoordgenerator tonen"));
    connect(actShow, &QAction::triggered, this, [this]() {
        m_mainWindow->showNormal();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
    });

    QAction* actQuickGen = m_trayMenu->addAction(IconUtils::getIcon(IconType::Refresh), QStringLiteral("Genereer & kopieer wachtwoord"));
    connect(actQuickGen, &QAction::triggered, this, &TrayManager::onQuickGenerateAndCopy);

    QAction* actVault = m_trayMenu->addAction(IconUtils::getIcon(IconType::Vault), QStringLiteral("Wachtwoordkluis openen"));
    connect(actVault, &QAction::triggered, this, [this]() {
        m_mainWindow->showNormal();
        m_mainWindow->raise();
        m_mainWindow->activateWindow();
        m_mainWindow->openVaultDialog();
    });

    m_trayMenu->addSeparator();

    QAction* actExit = m_trayMenu->addAction(IconUtils::getIcon(IconType::Trash), QStringLiteral("Afsluiten"));
    connect(actExit, &QAction::triggered, qApp, &QApplication::quit);

    m_trayIcon->setContextMenu(m_trayMenu);

    connect(m_trayIcon, &QSystemTrayIcon::activated, this, &TrayManager::onTrayIconActivated);

    m_trayIcon->show();
}

void TrayManager::onTrayIconActivated(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
        if (m_mainWindow->isVisible()) {
            m_mainWindow->hide();
        } else {
            m_mainWindow->showNormal();
            m_mainWindow->raise();
            m_mainWindow->activateWindow();
        }
    }
}

void TrayManager::onQuickGenerateAndCopy() {
    if (m_mainWindow) {
        m_mainWindow->quickGenerateAndCopy();
        showNotification(QStringLiteral("Wachtwoord gegenereerd"), 
                         QStringLiteral("Nieuw veilig wachtwoord is gegenereerd en gekopieerd naar klembord."));
    }
}

void TrayManager::showNotification(const QString& title, const QString& message, QSystemTrayIcon::MessageIcon icon) {
    if (m_trayIcon && m_trayIcon->isVisible()) {
        m_trayIcon->showMessage(title, message, icon, 3000);
    }
}

} // namespace gui
