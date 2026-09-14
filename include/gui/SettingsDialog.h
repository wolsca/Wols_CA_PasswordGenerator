#pragma once

#include <QDialog>
#include <QCheckBox>
#include <QRadioButton>
#include <QLineEdit>
#include <QSpinBox>
#include <QTabWidget>
#include <QLabel>
#include <QToolButton>
#include "core/PasswordOptions.h"
#include "core/VaultModel.h"

namespace gui {

class SettingsDialog : public QDialog {
    Q_OBJECT
public:
    explicit SettingsDialog(const core::VaultSettings& settings, QWidget* parent = nullptr);

    core::VaultSettings getSettings() const;
    core::PasswordOptions getOptions() const;

private:
    void setupUi();
    void setupGeneratorTab(QTabWidget* tabs);
    void setupStorageTab(QTabWidget* tabs);
    void setupSecurityTab(QTabWidget* tabs);
    void updateUiState();

    core::VaultSettings m_settings;

    // Generator widgets
    QCheckBox* m_chkLetters = nullptr;
    QRadioButton* m_rbBoth = nullptr;
    QRadioButton* m_rbLower = nullptr;
    QRadioButton* m_rbUpper = nullptr;

    QCheckBox* m_chkDigits = nullptr;
    QCheckBox* m_chkSpecial = nullptr;
    QLineEdit* m_editSpecialSet = nullptr;

    QCheckBox* m_chkHexOnly = nullptr;
    QRadioButton* m_rbHexUpper = nullptr;
    QRadioButton* m_rbHexLower = nullptr;

    QCheckBox* m_chkSignature = nullptr;
    QSpinBox* m_spinSigPos = nullptr;

    // Cloud / Storage widgets
    QRadioButton* m_rbOneDrive = nullptr;
    QRadioButton* m_rbGoogleDrive = nullptr;
    QRadioButton* m_rbLocal = nullptr;
    QRadioButton* m_rbCustom = nullptr;
    QLineEdit* m_editCustomPath = nullptr;
    QToolButton* m_btnBrowseCustom = nullptr;
    QLabel* m_lblOneDriveStatus = nullptr;
    QLabel* m_lblGoogleDriveStatus = nullptr;
    QLabel* m_lblResolvedPath = nullptr;

    // Security widgets
    QCheckBox* m_chkBioReveal = nullptr;
    QCheckBox* m_chkBioCopy = nullptr;
    QCheckBox* m_chkBioVault = nullptr;
    QLabel* m_lblBioStatus = nullptr;
};

} // namespace gui
