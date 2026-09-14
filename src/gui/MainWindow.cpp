#include "gui/MainWindow.h"
#include "gui/TrayManager.h"
#include "gui/SettingsDialog.h"
#include "gui/VaultDialog.h"
#include "gui/AccountEditDialog.h"
#include "gui/MasterPasswordDialog.h"
#include "gui/IconUtils.h"
#include "core/PasswordGenerator.h"
#include "core/VaultCrypto.h"
#include "core/VaultStorage.h"
#include "core/BiometricAuth.h"
#include <QFile>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QEvent>
#include <QCloseEvent>
#include <QWindowStateChangeEvent>
#include <QUuid>
#include <QShortcut>
#include <QKeySequence>

namespace gui {

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    setWindowTitle(QStringLiteral("Wols Password Generator"));
    setWindowIcon(IconUtils::getIcon(IconType::App));
    setFixedSize(540, 260);

    setupUi();
    loadVaultAndSettings();
    m_trayManager = new TrayManager(this);

    generateAndDisplayPassword();
}

MainWindow::~MainWindow() {
    saveSettingsToVault();
}

void MainWindow::setupUi() {
    QWidget* centralWidget = new QWidget(this);
    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(20, 20, 20, 16);
    mainLayout->setSpacing(14);

    // 1. Password Display Row + Hold to Reveal
    QHBoxLayout* passLayout = new QHBoxLayout();
    passLayout->setSpacing(8);

    m_editPassword = new QLineEdit(centralWidget);
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setReadOnly(true);
    m_editPassword->setStyleSheet("QLineEdit { font-family: 'Consolas', 'Courier New', monospace; font-size: 16px; font-weight: bold; padding: 6px 10px; border: 2px solid #ced4da; border-radius: 6px; } QLineEdit:focus { border-color: #007aff; }");
    passLayout->addWidget(m_editPassword, 1);

    m_btnReveal = new QToolButton(centralWidget);
    m_btnReveal->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
    m_btnReveal->setIconSize(QSize(24, 24));
    m_btnReveal->setFixedSize(38, 38);
    m_btnReveal->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
    m_btnReveal->setAutoRaise(true);
    passLayout->addWidget(m_btnReveal);

    mainLayout->addLayout(passLayout);

    // Hold to reveal logic
    connect(m_btnReveal, &QToolButton::pressed, this, [this]() {
        if (m_vaultDoc.settings.requireBiometricsForReveal) {
            if (!core::BiometricAuth::authenticate(QStringLiteral("Verifieer uw identiteit om het gegenereerde wachtwoord te tonen."), this)) {
                return;
            }
        }
        m_editPassword->setEchoMode(QLineEdit::Normal);
    });
    connect(m_btnReveal, &QToolButton::released, this, [this]() {
        m_editPassword->setEchoMode(QLineEdit::Password);
    });

    // 2. Length Slider & Spinbox Row
    QHBoxLayout* lengthLayout = new QHBoxLayout();
    QLabel* lblLength = new QLabel(QStringLiteral("Lengte:"), centralWidget);
    lblLength->setStyleSheet("font-weight: bold; font-size: 13px;");
    lengthLayout->addWidget(lblLength);

    m_sliderLength = new QSlider(Qt::Horizontal, centralWidget);
    m_sliderLength->setRange(8, 128);
    m_sliderLength->setValue(16);
    lengthLayout->addWidget(m_sliderLength, 1);

    m_spinLength = new QSpinBox(centralWidget);
    m_spinLength->setRange(8, 128);
    m_spinLength->setValue(16);
    m_spinLength->setFixedWidth(60);
    lengthLayout->addWidget(m_spinLength);

    mainLayout->addLayout(lengthLayout);

    connect(m_sliderLength, &QSlider::valueChanged, m_spinLength, &QSpinBox::setValue);
    connect(m_spinLength, &QSpinBox::valueChanged, m_sliderLength, &QSlider::setValue);
    connect(m_spinLength, &QSpinBox::valueChanged, this, [this](int val) {
        m_options.length = val;
        m_vaultDoc.settings.lastGeneratorLength = val;
        generateAndDisplayPassword();
    });

    // 3. Entropy & Strength meter
    QHBoxLayout* entropyLayout = new QHBoxLayout();
    entropyLayout->setSpacing(10);

    m_entropyBar = new QProgressBar(centralWidget);
    m_entropyBar->setRange(0, 128);
    m_entropyBar->setValue(80);
    m_entropyBar->setTextVisible(false);
    m_entropyBar->setFixedHeight(8);
    m_entropyBar->setStyleSheet("QProgressBar { background-color: #e9ecef; border-radius: 4px; } QProgressBar::chunk { background-color: #28a745; border-radius: 4px; }");
    entropyLayout->addWidget(m_entropyBar, 1);

    m_lblEntropyInfo = new QLabel(centralWidget);
    m_lblEntropyInfo->setStyleSheet("color: #666; font-size: 11px;");
    entropyLayout->addWidget(m_lblEntropyInfo);

    mainLayout->addLayout(entropyLayout);

    // 4. Action Buttons (Only Icons with Tooltips & Sneltoetsen)
    QHBoxLayout* btnLayout = new QHBoxLayout();
    btnLayout->setSpacing(12);

    m_btnGenerate = new QToolButton(centralWidget);
    m_btnGenerate->setIcon(IconUtils::getIcon(IconType::Refresh, QColor(0, 122, 255)));
    m_btnGenerate->setIconSize(QSize(24, 24));
    m_btnGenerate->setFixedSize(44, 40);
    m_btnGenerate->setToolTip(QStringLiteral("Nieuw wachtwoord genereren (F5 / Ctrl+G)"));
    m_btnGenerate->setShortcut(QKeySequence(Qt::Key_F5));
    btnLayout->addWidget(m_btnGenerate);

    m_btnCopy = new QToolButton(centralWidget);
    m_btnCopy->setIcon(IconUtils::getIcon(IconType::Copy, QColor(0, 122, 255)));
    m_btnCopy->setIconSize(QSize(24, 24));
    m_btnCopy->setFixedSize(44, 40);
    m_btnCopy->setToolTip(QStringLiteral("Kopieer wachtwoord naar klembord (Ctrl+C)"));
    m_btnCopy->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_C));
    btnLayout->addWidget(m_btnCopy);

    m_btnSaveToVault = new QToolButton(centralWidget);
    m_btnSaveToVault->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
    m_btnSaveToVault->setIconSize(QSize(24, 24));
    m_btnSaveToVault->setFixedSize(44, 40);
    m_btnSaveToVault->setToolTip(QStringLiteral("Wachtwoord opslaan in kluis (Ctrl+S)"));
    m_btnSaveToVault->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_S));
    btnLayout->addWidget(m_btnSaveToVault);

    btnLayout->addStretch();

    m_btnOpenVault = new QToolButton(centralWidget);
    m_btnOpenVault->setIcon(IconUtils::getIcon(IconType::Vault, QColor(70, 70, 70)));
    m_btnOpenVault->setIconSize(QSize(24, 24));
    m_btnOpenVault->setFixedSize(44, 40);
    m_btnOpenVault->setToolTip(QStringLiteral("Wachtwoordkluis openen (Ctrl+K)"));
    m_btnOpenVault->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_K));
    btnLayout->addWidget(m_btnOpenVault);

    m_btnSettings = new QToolButton(centralWidget);
    m_btnSettings->setIcon(IconUtils::getIcon(IconType::Settings, QColor(70, 70, 70)));
    m_btnSettings->setIconSize(QSize(24, 24));
    m_btnSettings->setFixedSize(44, 40);
    m_btnSettings->setToolTip(QStringLiteral("Instellingen, Cloud & Biometrie (Ctrl+, / F2)"));
    m_btnSettings->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_Comma));
    btnLayout->addWidget(m_btnSettings);

    mainLayout->addLayout(btnLayout);

    setCentralWidget(centralWidget);

    // Extra global shortcuts
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_G), this, [this]() { generateAndDisplayPassword(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_R), this, [this]() { generateAndDisplayPassword(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_O), this, [this]() { openVaultDialog(); });
    new QShortcut(QKeySequence(Qt::Key_F2), this, [this]() { m_btnSettings->click(); });

    // Connect button actions
    connect(m_btnGenerate, &QToolButton::clicked, this, &MainWindow::generateAndDisplayPassword);

    connect(m_btnCopy, &QToolButton::clicked, this, [this]() {
        if (m_vaultDoc.settings.requireBiometricsForCopy) {
            if (!core::BiometricAuth::authenticate(QStringLiteral("Verifieer uw identiteit om het wachtwoord naar het klembord te kopiëren."), this)) {
                return;
            }
        }
        QApplication::clipboard()->setText(m_editPassword->text());
        m_trayManager->showNotification(QStringLiteral("Gekopieerd"), 
                                        QStringLiteral("Wachtwoord is gekopieerd naar klembord."));
    });

    connect(m_btnSettings, &QToolButton::clicked, this, [this]() {
        SettingsDialog dlg(m_vaultDoc.settings, this);
        if (dlg.exec() == QDialog::Accepted) {
            core::VaultSettings newSettings = dlg.getSettings();
            m_options = newSettings.toPasswordOptions();
            m_vaultDoc.settings = newSettings;

            QString newPath = core::VaultStorage::resolveVaultPath(newSettings.preferredProvider, newSettings.customVaultPath, newSettings.selectedOneDriveDir);
            if (newPath != m_vaultPath) {
                if (QFile::exists(m_vaultPath) && !QFile::exists(newPath)) {
                    auto ret = QMessageBox::question(this, QStringLiteral("Kluis Verplaatsen"),
                        QStringLiteral("Wilt u de huidige kluis automatisch overzetten naar de nieuwe opslaglocatie?\n\nNieuwe locatie:\n%1").arg(newPath),
                        QMessageBox::Yes | QMessageBox::No);
                    if (ret == QMessageBox::Yes) {
                        QString err;
                        if (!core::VaultStorage::migrateVault(m_vaultPath, newPath, &err)) {
                            QMessageBox::warning(this, QStringLiteral("Fout bij migreren"), err);
                        }
                    }
                }
                m_vaultPath = newPath;
                m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
                m_vaultDoc.settings = newSettings;
            }

            saveSettingsToVault();
            generateAndDisplayPassword();
        }
    });

    connect(m_btnOpenVault, &QToolButton::clicked, this, &MainWindow::openVaultDialog);

    connect(m_btnSaveToVault, &QToolButton::clicked, this, [this]() {
        if (!unlockVaultIfNeeded()) return;

        QStringList groupNames;
        for (const auto& grp : m_vaultDoc.groups) {
            groupNames << grp.name;
        }
        if (groupNames.isEmpty()) {
            groupNames << QStringLiteral("Algemeen / General");
        }

        bool ok = false;
        QString chosenGroup = QInputDialog::getItem(this, QStringLiteral("Opslaan in Kluis"), 
                                                   QStringLiteral("Kies groep:"), groupNames, 0, false, &ok);
        if (!ok || chosenGroup.isEmpty()) return;

        // Find or create group
        core::VaultGroup* targetGrp = nullptr;
        for (auto& grp : m_vaultDoc.groups) {
            if (grp.name == chosenGroup) {
                targetGrp = &grp;
                break;
            }
        }
        if (!targetGrp) {
            core::VaultGroup newGrp;
            newGrp.id = "grp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            newGrp.name = chosenGroup;
            m_vaultDoc.groups.append(newGrp);
            targetGrp = &m_vaultDoc.groups.last();
        }

        QString itemTitle = QInputDialog::getText(this, QStringLiteral("Item Titel"), 
                                                 QStringLiteral("Voer titel/website in (bijv. Google, GitHub):"), 
                                                 QLineEdit::Normal, QString(), &ok);
        if (!ok || itemTitle.trimmed().isEmpty()) return;

        core::VaultItem newItem;
        newItem.id = "item_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        newItem.title = itemTitle.trimmed();
        newItem.createdAt = QDateTime::currentDateTimeUtc();
        newItem.updatedAt = newItem.createdAt;
        newItem.lastAccessed = newItem.createdAt;

        core::AccountEntry acc;
        acc.id = "acc_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        acc.label = QStringLiteral("Standaard account");
        acc.isDefaultEmail = true;
        acc.lastAccessed = newItem.createdAt;

        AccountEditDialog accDlg(acc, m_editPassword->text(), m_options, this);
        if (accDlg.exec() == QDialog::Accepted) {
            acc = accDlg.getAccount();
            QString pass = accDlg.getPassword();
            core::VaultCrypto::encryptPassword(pass, m_masterKey, acc.encryptedPassword, acc.nonce, acc.authTag);
            newItem.accounts.append(acc);
            targetGrp->items.append(newItem);
            m_vaultDoc.lastSynced = QDateTime::currentDateTimeUtc();
            core::VaultStorage::saveVault(m_vaultPath, m_vaultDoc);

            QMessageBox::information(this, QStringLiteral("Opgeslagen"), 
                                     QString("Wachtwoord voor '%1' is succesvol versleuteld en opgeslagen in de kluis.").arg(newItem.title));
        }
    });
}

void MainWindow::loadVaultAndSettings() {
    QString configVaultPath;
    core::CloudProvider provider = core::CloudProvider::OneDrive;
    QString selectedOneDriveDir;
    core::VaultSettings localSettings;

    bool hasLocalConfig = core::VaultStorage::loadLocalConfig(configVaultPath, provider, selectedOneDriveDir, localSettings);

    // Scan for any existing populated vault file (in Google Drive, OneDrive, or Local Documents)
    core::CloudProvider detectedProvider = core::CloudProvider::OneDrive;
    QString detectedOneDriveDir;
    QString existingVault = core::VaultStorage::findExistingVault(&detectedProvider, &detectedOneDriveDir);

    if (hasLocalConfig && !configVaultPath.isEmpty() && QFile::exists(configVaultPath)) {
        m_vaultPath = configVaultPath;
        m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
        m_vaultDoc.settings = localSettings;
        if (!selectedOneDriveDir.isEmpty()) {
            m_vaultDoc.settings.selectedOneDriveDir = selectedOneDriveDir;
        }

        // If configured vault file is completely empty/uninitialized, but an existing populated vault was found elsewhere (e.g. Google Drive)
        if (m_vaultDoc.kdfSalt.isEmpty() && m_vaultDoc.groups.isEmpty() && !existingVault.isEmpty() && existingVault != configVaultPath) {
            m_vaultPath = existingVault;
            m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
            m_vaultDoc.settings.preferredProvider = detectedProvider;
            if (!detectedOneDriveDir.isEmpty()) {
                m_vaultDoc.settings.selectedOneDriveDir = detectedOneDriveDir;
            }
            core::VaultStorage::saveLocalConfig(m_vaultPath, m_vaultDoc.settings.preferredProvider, m_vaultDoc.settings.selectedOneDriveDir, m_vaultDoc.settings);
        }
    } else if (!existingVault.isEmpty()) {
        // An existing vault was found on disk (e.g. in Google Drive or OneDrive)
        m_vaultPath = existingVault;
        m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
        if (hasLocalConfig) {
            m_vaultDoc.settings = localSettings;
        }
        m_vaultDoc.settings.preferredProvider = detectedProvider;
        if (!detectedOneDriveDir.isEmpty()) {
            m_vaultDoc.settings.selectedOneDriveDir = detectedOneDriveDir;
        }
        core::VaultStorage::saveLocalConfig(m_vaultPath, m_vaultDoc.settings.preferredProvider, m_vaultDoc.settings.selectedOneDriveDir, m_vaultDoc.settings);
    } else if (hasLocalConfig) {
        // Local config exists but active vault path needs resolution
        m_vaultDoc.settings = localSettings;
        if (!selectedOneDriveDir.isEmpty()) {
            m_vaultDoc.settings.selectedOneDriveDir = selectedOneDriveDir;
        }
        m_vaultPath = core::VaultStorage::resolveVaultPath(provider, localSettings.customVaultPath, m_vaultDoc.settings.selectedOneDriveDir);
        m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
        m_vaultDoc.settings = localSettings;
        core::VaultStorage::saveLocalConfig(m_vaultPath, m_vaultDoc.settings.preferredProvider, m_vaultDoc.settings.selectedOneDriveDir, m_vaultDoc.settings);
    } else {
        // Startup resolution sequence for new setup without existing vault:
        // 1. Check Google Drive if available
        if (core::VaultStorage::isCloudPathAvailable(core::CloudProvider::GoogleDrive)) {
            m_vaultDoc.settings.preferredProvider = core::CloudProvider::GoogleDrive;
            m_vaultPath = core::VaultStorage::getGoogleDriveVaultPath();
            m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
        } else {
            // 2. Check OneDrive(s)
            QStringList availableOneDrives = core::VaultStorage::getAvailableOneDriveDirectories();
            if (availableOneDrives.size() > 1) {
                bool ok = false;
                QString chosen = QInputDialog::getItem(this, QStringLiteral("Selecteer OneDrive Account"),
                    QStringLiteral("Er zijn meerdere OneDrive locaties gedetecteerd op dit systeem.\nKies welke OneDrive map u wilt gebruiken voor synchronisatie:"),
                    availableOneDrives, 0, false, &ok);
                if (ok && !chosen.isEmpty()) {
                    selectedOneDriveDir = chosen;
                }
            } else if (!availableOneDrives.isEmpty()) {
                selectedOneDriveDir = availableOneDrives.first();
            }

            if (!availableOneDrives.isEmpty()) {
                m_vaultDoc.settings.preferredProvider = core::CloudProvider::OneDrive;
                m_vaultDoc.settings.selectedOneDriveDir = selectedOneDriveDir;
                m_vaultPath = core::VaultStorage::getOneDriveVaultPath(selectedOneDriveDir);
                m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
            } else {
                // 3. Fallback to Local Documents
                m_vaultDoc.settings.preferredProvider = core::CloudProvider::Local;
                m_vaultPath = core::VaultStorage::getLocalVaultPath();
                m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
            }
        }

        // Save initial local configuration file in user directory
        core::VaultStorage::saveLocalConfig(m_vaultPath, m_vaultDoc.settings.preferredProvider, m_vaultDoc.settings.selectedOneDriveDir, m_vaultDoc.settings);
    }

    m_options = m_vaultDoc.settings.toPasswordOptions();
    m_spinLength->setValue(m_options.length);
    m_sliderLength->setValue(m_options.length);
}

void MainWindow::saveSettingsToVault() {
    m_vaultDoc.settings.fromPasswordOptions(m_options);
    m_vaultDoc.lastSynced = QDateTime::currentDateTimeUtc();
    core::VaultStorage::saveVault(m_vaultPath, m_vaultDoc);
    core::VaultStorage::saveLocalConfig(m_vaultPath, m_vaultDoc.settings.preferredProvider, m_vaultDoc.settings.selectedOneDriveDir, m_vaultDoc.settings);
}

bool MainWindow::unlockVaultIfNeeded() {
    if (m_vaultUnlocked && !m_masterKey.isEmpty()) {
        return true;
    }

    bool isInitial = m_vaultDoc.kdfSalt.isEmpty() || m_vaultDoc.groups.isEmpty();
    MasterPasswordDialog dlg(isInitial, this);
    if (dlg.exec() == QDialog::Accepted) {
        QString masterPass = dlg.getMasterPassword();
        if (m_vaultDoc.kdfSalt.isEmpty()) {
            m_vaultDoc.kdfSalt = core::VaultCrypto::generateSalt();
        }
        m_masterKey = core::VaultCrypto::deriveKey(masterPass, m_vaultDoc.kdfSalt, m_vaultDoc.kdfIterations);
        m_vaultUnlocked = true;
        return true;
    }
    return false;
}

void MainWindow::generateAndDisplayPassword() {
    std::string generated = core::PasswordGenerator::generate(m_options);
    m_editPassword->setText(QString::fromStdString(generated));
    updateEntropyDisplay();
}

void MainWindow::quickGenerateAndCopy() {
    std::string generated = core::PasswordGenerator::generate(m_options);
    m_editPassword->setText(QString::fromStdString(generated));
    QApplication::clipboard()->setText(QString::fromStdString(generated));
    updateEntropyDisplay();
}

void MainWindow::updateEntropyDisplay() {
    double entropy = core::PasswordGenerator::calculateEntropy(m_options);
    int intEntropy = static_cast<int>(entropy);
    m_entropyBar->setValue(qBound(0, intEntropy, 128));

    QString level;
    QString color;
    if (entropy < 50) {
        level = QStringLiteral("Zwak / Weak");
        color = "#dc3545";
    } else if (entropy < 75) {
        level = QStringLiteral("Redelijk / Fair");
        color = "#fd7e14";
    } else {
        level = QStringLiteral("Sterk / Strong");
        color = "#28a745";
    }

    m_entropyBar->setStyleSheet(QString("QProgressBar { background-color: #e9ecef; border-radius: 4px; } QProgressBar::chunk { background-color: %1; border-radius: 4px; }").arg(color));
    m_lblEntropyInfo->setText(QString("%1 bits (%2)").arg(QString::number(entropy, 'f', 1), level));
}

void MainWindow::openVaultDialog() {
    if (m_vaultDoc.settings.requireBiometricsForVault) {
        if (!core::BiometricAuth::authenticate(QStringLiteral("Verifieer uw identiteit om de wachtwoordkluis te openen."), this)) {
            return;
        }
    }

    if (!unlockVaultIfNeeded()) return;

    VaultDialog dlg(m_vaultPath, m_masterKey, m_vaultDoc, this);
    dlg.exec();
    m_vaultDoc = dlg.getVaultDocument();
}

void MainWindow::changeEvent(QEvent* event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (isMinimized()) {
            hide();
            m_trayManager->showNotification(QStringLiteral("Wols Password Generator"), 
                                            QStringLiteral("De applicatie draait in het systeemvak."));
            event->accept();
            return;
        }
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    saveSettingsToVault();
    event->accept();
}

} // namespace gui
