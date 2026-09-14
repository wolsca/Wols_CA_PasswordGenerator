#include "gui/SettingsDialog.h"
#include "gui/IconUtils.h"
#include "core/VaultStorage.h"
#include "core/BiometricAuth.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QLabel>
#include <QDialogButtonBox>
#include <QButtonGroup>
#include <QFileDialog>
#include <QFileInfo>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>

namespace gui {

SettingsDialog::SettingsDialog(const core::VaultSettings& settings, QWidget* parent)
    : QDialog(parent), m_settings(settings)
{
    setWindowTitle(QStringLiteral("Instellingen / Settings"));
    setWindowIcon(IconUtils::getIcon(IconType::Settings, QColor(0, 122, 255)));
    setMinimumWidth(500);
    resize(520, 420);
    setupUi();
}

void SettingsDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QTabWidget* tabs = new QTabWidget(this);
    setupGeneratorTab(tabs);
    setupStorageTab(tabs);
    setupSecurityTab(tabs);
    setupPlatformConfigTab(tabs);
    mainLayout->addWidget(tabs);

    // Dialog buttons
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    updateUiState();
    updatePlatformConfigView();
}

void SettingsDialog::setupGeneratorTab(QTabWidget* tabs) {
    QWidget* genWidget = new QWidget(tabs);
    QVBoxLayout* genLayout = new QVBoxLayout(genWidget);
    genLayout->setSpacing(12);

    core::PasswordOptions opt = m_settings.toPasswordOptions();

    // Character Sets Group
    QGroupBox* charsetGroup = new QGroupBox(QStringLiteral("Karaktersets / Character Sets"), genWidget);
    QVBoxLayout* charsetLayout = new QVBoxLayout(charsetGroup);

    m_chkLetters = new QCheckBox(QStringLiteral("Letters (a-z, A-Z)"), charsetGroup);
    m_chkLetters->setChecked(opt.useLetters);
    charsetLayout->addWidget(m_chkLetters);

    QHBoxLayout* letterCaseLayout = new QHBoxLayout();
    letterCaseLayout->setContentsMargins(20, 0, 0, 0);
    m_rbBoth = new QRadioButton(QStringLiteral("Beide (a-z, A-Z)"), charsetGroup);
    m_rbLower = new QRadioButton(QStringLiteral("Alleen klein (a-z)"), charsetGroup);
    m_rbUpper = new QRadioButton(QStringLiteral("Alleen groot (A-Z)"), charsetGroup);

    QButtonGroup* letterGroup = new QButtonGroup(charsetGroup);
    letterGroup->addButton(m_rbBoth);
    letterGroup->addButton(m_rbLower);
    letterGroup->addButton(m_rbUpper);

    if (opt.letterCase == core::LetterCase::LowerOnly) m_rbLower->setChecked(true);
    else if (opt.letterCase == core::LetterCase::UpperOnly) m_rbUpper->setChecked(true);
    else m_rbBoth->setChecked(true);

    letterCaseLayout->addWidget(m_rbBoth);
    letterCaseLayout->addWidget(m_rbLower);
    letterCaseLayout->addWidget(m_rbUpper);
    charsetLayout->addLayout(letterCaseLayout);

    m_chkDigits = new QCheckBox(QStringLiteral("Cijfers (0-9)"), charsetGroup);
    m_chkDigits->setChecked(opt.useDigits);
    charsetLayout->addWidget(m_chkDigits);

    m_chkSpecial = new QCheckBox(QStringLiteral("Speciale tekens / Symbols"), charsetGroup);
    m_chkSpecial->setChecked(opt.useSpecialChars);
    charsetLayout->addWidget(m_chkSpecial);

    QHBoxLayout* specialSetLayout = new QHBoxLayout();
    specialSetLayout->setContentsMargins(20, 0, 0, 0);
    QLabel* specialLabel = new QLabel(QStringLiteral("Tekenset:"), charsetGroup);
    m_editSpecialSet = new QLineEdit(charsetGroup);
    m_editSpecialSet->setText(QString::fromStdString(opt.specialCharSet));
    specialSetLayout->addWidget(specialLabel);
    specialSetLayout->addWidget(m_editSpecialSet);
    charsetLayout->addLayout(specialSetLayout);

    genLayout->addWidget(charsetGroup);

    // Hexadecimal Group
    QGroupBox* hexGroup = new QGroupBox(QStringLiteral("Hexadecimaal / Hexadecimal"), genWidget);
    QVBoxLayout* hexLayout = new QVBoxLayout(hexGroup);

    m_chkHexOnly = new QCheckBox(QStringLiteral("Alleen Hexadecimaal (overschrijft overige sets)"), hexGroup);
    m_chkHexOnly->setChecked(opt.hexOnly);
    hexLayout->addWidget(m_chkHexOnly);

    QHBoxLayout* hexCaseLayout = new QHBoxLayout();
    hexCaseLayout->setContentsMargins(20, 0, 0, 0);
    m_rbHexUpper = new QRadioButton(QStringLiteral("Hoofdletters (0-9, A-F)"), hexGroup);
    m_rbHexLower = new QRadioButton(QStringLiteral("Kleine letters (0-9, a-f)"), hexGroup);

    QButtonGroup* hexButtonGroup = new QButtonGroup(hexGroup);
    hexButtonGroup->addButton(m_rbHexUpper);
    hexButtonGroup->addButton(m_rbHexLower);

    if (opt.hexCase == core::HexCase::Lowercase) m_rbHexLower->setChecked(true);
    else m_rbHexUpper->setChecked(true);

    hexCaseLayout->addWidget(m_rbHexUpper);
    hexCaseLayout->addWidget(m_rbHexLower);
    hexLayout->addLayout(hexCaseLayout);

    genLayout->addWidget(hexGroup);

    // Signature Group
    QGroupBox* sigGroup = new QGroupBox(QStringLiteral("Handtekening / Signature Feature"), genWidget);
    QVBoxLayout* sigLayout = new QVBoxLayout(sigGroup);

    m_chkSignature = new QCheckBox(QStringLiteral("Handtekening inschakelen (speciaal teken op vaste positie)"), sigGroup);
    m_chkSignature->setChecked(opt.useSignature);
    sigLayout->addWidget(m_chkSignature);

    QHBoxLayout* sigPosLayout = new QHBoxLayout();
    sigPosLayout->setContentsMargins(20, 0, 0, 0);
    QLabel* sigPosLabel = new QLabel(QStringLiteral("Positie (1-gebaseerd):"), sigGroup);
    m_spinSigPos = new QSpinBox(sigGroup);
    m_spinSigPos->setRange(1, opt.length > 0 ? opt.length : 128);
    m_spinSigPos->setValue(opt.signaturePosition);
    sigPosLayout->addWidget(sigPosLabel);
    sigPosLayout->addWidget(m_spinSigPos);
    sigPosLayout->addStretch();
    sigLayout->addLayout(sigPosLayout);

    genLayout->addWidget(sigGroup);
    genLayout->addStretch();

    tabs->addTab(genWidget, QStringLiteral("Generator"));

    connect(m_chkLetters, &QCheckBox::toggled, this, &SettingsDialog::updateUiState);
    connect(m_chkSpecial, &QCheckBox::toggled, this, &SettingsDialog::updateUiState);
    connect(m_chkHexOnly, &QCheckBox::toggled, this, &SettingsDialog::updateUiState);
    connect(m_chkSignature, &QCheckBox::toggled, this, &SettingsDialog::updateUiState);
}

void SettingsDialog::setupStorageTab(QTabWidget* tabs) {
    QWidget* storageWidget = new QWidget(tabs);
    QVBoxLayout* storageLayout = new QVBoxLayout(storageWidget);
    storageLayout->setSpacing(14);

    QGroupBox* providerGroup = new QGroupBox(QStringLiteral("Kluis Opslaglocatie / Vault Storage Location"), storageWidget);
    QVBoxLayout* provLayout = new QVBoxLayout(providerGroup);
    provLayout->setSpacing(10);

    QButtonGroup* provBtnGroup = new QButtonGroup(providerGroup);

    // 1. OneDrive
    QStringList availableOneDrives = core::VaultStorage::getAvailableOneDriveDirectories();
    bool oneDriveAvail = !availableOneDrives.isEmpty();
    m_rbOneDrive = new QRadioButton(QStringLiteral("Microsoft OneDrive (Aanbevolen voor cloud sync)"), providerGroup);
    provBtnGroup->addButton(m_rbOneDrive);
    provLayout->addWidget(m_rbOneDrive);

    m_lblOneDriveStatus = new QLabel(providerGroup);
    m_lblOneDriveStatus->setStyleSheet(oneDriveAvail ? "color: #28a745; margin-left: 22px;" : "color: #888; margin-left: 22px;");
    m_lblOneDriveStatus->setText(oneDriveAvail ? QStringLiteral("✓ Gedetecteerd (%1 account(s))").arg(availableOneDrives.size()) 
                                              : QStringLiteral("✕ Niet gedetecteerd op dit systeem"));
    provLayout->addWidget(m_lblOneDriveStatus);

    if (oneDriveAvail) {
        QHBoxLayout* odSelectLayout = new QHBoxLayout();
        odSelectLayout->setContentsMargins(22, 0, 0, 0);
        QLabel* odLabel = new QLabel(QStringLiteral("OneDrive account/map:"), providerGroup);
        m_comboOneDriveAccounts = new QComboBox(providerGroup);
        for (const QString& d : availableOneDrives) {
            m_comboOneDriveAccounts->addItem(d);
        }
        if (!m_settings.selectedOneDriveDir.isEmpty()) {
            int idx = m_comboOneDriveAccounts->findText(m_settings.selectedOneDriveDir);
            if (idx >= 0) m_comboOneDriveAccounts->setCurrentIndex(idx);
        }
        odSelectLayout->addWidget(odLabel);
        odSelectLayout->addWidget(m_comboOneDriveAccounts, 1);
        provLayout->addLayout(odSelectLayout);
    }

    // 2. Google Drive
    QString gDriveDir = core::VaultStorage::getGoogleDriveDirectory();
    bool gDriveAvail = !gDriveDir.isEmpty();
    m_rbGoogleDrive = new QRadioButton(QStringLiteral("Google Drive (Desktop sync)"), providerGroup);
    provBtnGroup->addButton(m_rbGoogleDrive);
    provLayout->addWidget(m_rbGoogleDrive);

    m_lblGoogleDriveStatus = new QLabel(providerGroup);
    m_lblGoogleDriveStatus->setStyleSheet(gDriveAvail ? "color: #28a745; margin-left: 22px;" : "color: #888; margin-left: 22px;");
    m_lblGoogleDriveStatus->setText(gDriveAvail ? QStringLiteral("✓ Gedetecteerd: ") + gDriveDir 
                                                : QStringLiteral("✕ Niet gedetecteerd op dit systeem"));
    provLayout->addWidget(m_lblGoogleDriveStatus);

    // 3. Local Documents
    m_rbLocal = new QRadioButton(QStringLiteral("Lokaal (Documenten / AppData)"), providerGroup);
    provBtnGroup->addButton(m_rbLocal);
    provLayout->addWidget(m_rbLocal);

    // 4. Custom Path
    m_rbCustom = new QRadioButton(QStringLiteral("Aangepaste map / bestand..."), providerGroup);
    provBtnGroup->addButton(m_rbCustom);
    provLayout->addWidget(m_rbCustom);

    QHBoxLayout* customPathLayout = new QHBoxLayout();
    customPathLayout->setContentsMargins(22, 0, 0, 0);
    m_editCustomPath = new QLineEdit(providerGroup);
    m_editCustomPath->setText(m_settings.customVaultPath);
    m_editCustomPath->setPlaceholderText(QStringLiteral("Kies een aangepast kluispad (*.json)"));
    m_btnBrowseCustom = new QToolButton(providerGroup);
    m_btnBrowseCustom->setText(QStringLiteral("..."));
    customPathLayout->addWidget(m_editCustomPath, 1);
    customPathLayout->addWidget(m_btnBrowseCustom);
    provLayout->addLayout(customPathLayout);

    storageLayout->addWidget(providerGroup);

    // Active resolved path box
    QGroupBox* infoGroup = new QGroupBox(QStringLiteral("Actief kluisbestand"), storageWidget);
    QVBoxLayout* infoLayout = new QVBoxLayout(infoGroup);
    m_lblResolvedPath = new QLabel(infoGroup);
    m_lblResolvedPath->setWordWrap(true);
    m_lblResolvedPath->setStyleSheet("font-family: 'Consolas', monospace; color: #1a73e8; font-size: 11px;");
    infoLayout->addWidget(m_lblResolvedPath);
    storageLayout->addWidget(infoGroup);

    storageLayout->addStretch();

    tabs->addTab(storageWidget, QStringLiteral("Cloud & Opslag"));

    // Select initial radio
    switch (m_settings.preferredProvider) {
        case core::CloudProvider::OneDrive:
            m_rbOneDrive->setChecked(true);
            break;
        case core::CloudProvider::GoogleDrive:
            m_rbGoogleDrive->setChecked(true);
            break;
        case core::CloudProvider::Custom:
            m_rbCustom->setChecked(true);
            break;
        case core::CloudProvider::Local:
        default:
            m_rbLocal->setChecked(true);
            break;
    }

    auto updateResolvedLabel = [this]() {
        core::CloudProvider p = core::CloudProvider::Local;
        if (m_rbOneDrive->isChecked()) p = core::CloudProvider::OneDrive;
        else if (m_rbGoogleDrive->isChecked()) p = core::CloudProvider::GoogleDrive;
        else if (m_rbCustom->isChecked()) p = core::CloudProvider::Custom;

        QString selectedOD = m_comboOneDriveAccounts ? m_comboOneDriveAccounts->currentText() : QString();
        QString path = core::VaultStorage::resolveVaultPath(p, m_editCustomPath->text(), selectedOD);
        m_lblResolvedPath->setText(path);
        m_editCustomPath->setEnabled(m_rbCustom->isChecked());
        m_btnBrowseCustom->setEnabled(m_rbCustom->isChecked());
        if (m_comboOneDriveAccounts) {
            m_comboOneDriveAccounts->setEnabled(m_rbOneDrive->isChecked());
        }
        updatePlatformConfigView();
    };

    connect(m_rbOneDrive, &QRadioButton::toggled, this, updateResolvedLabel);
    if (m_comboOneDriveAccounts) {
        connect(m_comboOneDriveAccounts, &QComboBox::currentTextChanged, this, updateResolvedLabel);
    }
    connect(m_rbGoogleDrive, &QRadioButton::toggled, this, updateResolvedLabel);
    connect(m_rbLocal, &QRadioButton::toggled, this, updateResolvedLabel);
    connect(m_rbCustom, &QRadioButton::toggled, this, updateResolvedLabel);
    connect(m_editCustomPath, &QLineEdit::textChanged, this, updateResolvedLabel);

    connect(m_btnBrowseCustom, &QToolButton::clicked, this, [this, updateResolvedLabel]() {
        QString file = QFileDialog::getSaveFileName(this, QStringLiteral("Selecteer kluisbestand"), 
                                                   QString(), QStringLiteral("JSON Kluisbestanden (*.json);;Alle bestanden (*.*)"));
        if (!file.isEmpty()) {
            m_editCustomPath->setText(file);
            updateResolvedLabel();
        }
    });

    updateResolvedLabel();
}

void SettingsDialog::setupSecurityTab(QTabWidget* tabs) {
    QWidget* secWidget = new QWidget(tabs);
    QVBoxLayout* secLayout = new QVBoxLayout(secWidget);
    secLayout->setSpacing(14);

    QGroupBox* bioGroup = new QGroupBox(QStringLiteral("Windows Hello & Biometrie"), secWidget);
    QVBoxLayout* bioLayout = new QVBoxLayout(bioGroup);
    bioLayout->setSpacing(10);

    bool bioAvailable = core::BiometricAuth::isAvailable();
    m_lblBioStatus = new QLabel(bioGroup);
    m_lblBioStatus->setStyleSheet(bioAvailable ? "font-weight: bold; color: #28a745;" : "font-weight: bold; color: #888;");
    m_lblBioStatus->setText(bioAvailable 
        ? QStringLiteral("✓ Windows Hello / OS-authenticatie (PIN, Vingerafdruk, Gezicht) is gereed.") 
        : QStringLiteral("✕ Biometrische authenticatie is niet beschikbaar op dit platform."));
    bioLayout->addWidget(m_lblBioStatus);

    m_chkBioReveal = new QCheckBox(QStringLiteral("Windows Hello verificatie vereisen bij wachtwoord tonen ('Hold to Reveal')"), bioGroup);
    m_chkBioReveal->setChecked(m_settings.requireBiometricsForReveal);
    bioLayout->addWidget(m_chkBioReveal);

    m_chkBioCopy = new QCheckBox(QStringLiteral("Windows Hello verificatie vereisen bij wachtwoord kopiëren naar klembord"), bioGroup);
    m_chkBioCopy->setChecked(m_settings.requireBiometricsForCopy);
    bioLayout->addWidget(m_chkBioCopy);

    m_chkBioVault = new QCheckBox(QStringLiteral("Windows Hello verificatie vereisen bij het openen van de kluis"), bioGroup);
    m_chkBioVault->setChecked(m_settings.requireBiometricsForVault);
    bioLayout->addWidget(m_chkBioVault);

    secLayout->addWidget(bioGroup);

    QGroupBox* hintGroup = new QGroupBox(QStringLiteral("Toelichting beveiliging"), secWidget);
    QVBoxLayout* hintLayout = new QVBoxLayout(hintGroup);
    QLabel* hintLabel = new QLabel(QStringLiteral(
        "• Met Windows Hello voorkomt u dat onbevoegden bij een ontgrendelde pc uw wachtwoorden kunnen inzien of kopiëren.\n"
        "• De master key blijft veilig versleuteld in het geheugen volgens de hoogste standaarden (AES-256-GCM + PBKDF2)."
    ), hintGroup);
    hintLabel->setWordWrap(true);
    hintLabel->setStyleSheet("color: #555; font-size: 11px;");
    hintLayout->addWidget(hintLabel);
    secLayout->addWidget(hintGroup);

    secLayout->addStretch();
    tabs->addTab(secWidget, QStringLiteral("Beveiliging & Biometrie"));
}

void SettingsDialog::setupPlatformConfigTab(QTabWidget* tabs) {
    QWidget* platWidget = new QWidget(tabs);
    QVBoxLayout* platLayout = new QVBoxLayout(platWidget);
    platLayout->setSpacing(10);

    // Info header
    QGroupBox* localConfigBox = new QGroupBox(QStringLiteral("Lokale OS Configuratiebestand"), platWidget);
    QVBoxLayout* lcLayout = new QVBoxLayout(localConfigBox);
    m_lblLocalConfigPath = new QLabel(localConfigBox);
    m_lblLocalConfigPath->setText(QString("<b>Pad in OS User Directory:</b><br>%1").arg(core::VaultStorage::getLocalConfigPath()));
    m_lblLocalConfigPath->setWordWrap(true);
    m_lblLocalConfigPath->setStyleSheet("color: #1e293b; font-size: 11px;");
    lcLayout->addWidget(m_lblLocalConfigPath);
    platLayout->addWidget(localConfigBox);

    // Platform Selector & Copy Tools
    QHBoxLayout* selLayout = new QHBoxLayout();
    QLabel* lblSel = new QLabel(QStringLiteral("Doelplatform / Config:"), platWidget);
    lblSel->setStyleSheet("font-weight: bold;");
    m_comboPlatformSelect = new QComboBox(platWidget);
    m_comboPlatformSelect->addItem(QStringLiteral("Alle Platforms (Volledige Config)"), "all");
    m_comboPlatformSelect->addItem(QStringLiteral("Windows"), "windows");
    m_comboPlatformSelect->addItem(QStringLiteral("Linux"), "linux");
    m_comboPlatformSelect->addItem(QStringLiteral("Android"), "android");
    selLayout->addWidget(lblSel);
    selLayout->addWidget(m_comboPlatformSelect, 1);

    QToolButton* btnCopyConfig = new QToolButton(platWidget);
    btnCopyConfig->setIcon(IconUtils::getIcon(IconType::Copy, QColor(0, 122, 255)));
    btnCopyConfig->setText(QStringLiteral(" Kopiëren"));
    btnCopyConfig->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnCopyConfig->setToolTip(QStringLiteral("Kopieer de gegenereerde JSON configuratie naar het klembord"));
    selLayout->addWidget(btnCopyConfig);

    QToolButton* btnExportConfig = new QToolButton(platWidget);
    btnExportConfig->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
    btnExportConfig->setText(QStringLiteral(" Exporteren..."));
    btnExportConfig->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    btnExportConfig->setToolTip(QStringLiteral("Exporteer configuratie als config.json bestand"));
    selLayout->addWidget(btnExportConfig);

    platLayout->addLayout(selLayout);

    m_txtPlatformConfig = new QPlainTextEdit(platWidget);
    m_txtPlatformConfig->setReadOnly(true);
    m_txtPlatformConfig->setStyleSheet("font-family: 'Consolas', 'Courier New', monospace; font-size: 11px; background-color: #f8fafc; border: 1px solid #cbd5e1; border-radius: 4px;");
    platLayout->addWidget(m_txtPlatformConfig, 1);

    tabs->addTab(platWidget, QStringLiteral("Platform Configs"));

    connect(m_comboPlatformSelect, &QComboBox::currentIndexChanged, this, [this]() {
        updatePlatformConfigView();
    });

    connect(btnCopyConfig, &QToolButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_txtPlatformConfig->toPlainText());
        QMessageBox::information(this, QStringLiteral("Gekopieerd"), QStringLiteral("De platformconfiguratie is naar het klembord gekopieerd."));
    });

    connect(btnExportConfig, &QToolButton::clicked, this, [this]() {
        QString defaultName = "config.json";
        QString file = QFileDialog::getSaveFileName(this, QStringLiteral("Configuratie Exporteren"), 
                                                   defaultName, QStringLiteral("JSON Configuratie (*.json);;Alle bestanden (*.*)"));
        if (!file.isEmpty()) {
            QFile outFile(file);
            if (outFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
                outFile.write(m_txtPlatformConfig->toPlainText().toUtf8());
                outFile.close();
                QMessageBox::information(this, QStringLiteral("Opgeslagen"), QStringLiteral("Configuratiebestand succesvol opgeslagen."));
            } else {
                QMessageBox::warning(this, QStringLiteral("Fout"), QStringLiteral("Kan bestand niet opslaan: ") + outFile.errorString());
            }
        }
    });
}

void SettingsDialog::updatePlatformConfigView() {
    if (!m_txtPlatformConfig || !m_comboPlatformSelect) return;

    core::VaultSettings current = getSettings();
    QString targetPlat = m_comboPlatformSelect->currentData().toString();
    if (targetPlat.isEmpty()) targetPlat = "all";

    QString selectedOD = m_comboOneDriveAccounts ? m_comboOneDriveAccounts->currentText() : current.selectedOneDriveDir;
    QString activeVault = core::VaultStorage::resolveVaultPath(current.preferredProvider, current.customVaultPath, selectedOD);

    QString jsonStr = core::VaultStorage::generatePlatformConfigJsonString(targetPlat, activeVault, current.preferredProvider, selectedOD, current);
    m_txtPlatformConfig->setPlainText(jsonStr);
}

void SettingsDialog::updateUiState() {
    bool hex = m_chkHexOnly->isChecked();
    m_rbHexUpper->setEnabled(hex);
    m_rbHexLower->setEnabled(hex);

    m_chkLetters->setEnabled(!hex);
    m_rbBoth->setEnabled(!hex && m_chkLetters->isChecked());
    m_rbLower->setEnabled(!hex && m_chkLetters->isChecked());
    m_rbUpper->setEnabled(!hex && m_chkLetters->isChecked());

    m_chkDigits->setEnabled(!hex);
    m_chkSpecial->setEnabled(!hex);
    m_editSpecialSet->setEnabled(!hex && m_chkSpecial->isChecked());

    bool sig = m_chkSignature->isChecked();
    m_spinSigPos->setEnabled(sig);

    updatePlatformConfigView();
}

core::PasswordOptions SettingsDialog::getOptions() const {
    return getSettings().toPasswordOptions();
}

core::VaultSettings SettingsDialog::getSettings() const {
    core::VaultSettings s = m_settings;

    s.useLetters = m_chkLetters->isChecked();
    if (m_rbLower->isChecked()) s.letterCase = 1;
    else if (m_rbUpper->isChecked()) s.letterCase = 2;
    else s.letterCase = 0;

    s.useDigits = m_chkDigits->isChecked();
    s.useSpecialChars = m_chkSpecial->isChecked();
    s.specialCharSet = m_editSpecialSet->text();

    s.hexOnly = m_chkHexOnly->isChecked();
    s.hexCase = m_rbHexLower->isChecked() ? 1 : 0;

    s.useSignature = m_chkSignature->isChecked();
    s.signaturePosition = m_spinSigPos->value();

    if (m_rbOneDrive->isChecked()) s.preferredProvider = core::CloudProvider::OneDrive;
    else if (m_rbGoogleDrive->isChecked()) s.preferredProvider = core::CloudProvider::GoogleDrive;
    else if (m_rbCustom->isChecked()) s.preferredProvider = core::CloudProvider::Custom;
    else s.preferredProvider = core::CloudProvider::Local;

    s.customVaultPath = m_editCustomPath->text().trimmed();
    if (m_comboOneDriveAccounts) {
        s.selectedOneDriveDir = m_comboOneDriveAccounts->currentText().trimmed();
    }

    s.requireBiometricsForReveal = m_chkBioReveal->isChecked();
    s.requireBiometricsForCopy = m_chkBioCopy->isChecked();
    s.requireBiometricsForVault = m_chkBioVault->isChecked();

    return s;
}

} // namespace gui
