#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QSlider>
#include <QSpinBox>
#include <QToolButton>
#include <QLabel>
#include <QProgressBar>
#include "core/PasswordOptions.h"
#include "core/VaultModel.h"

namespace gui {

class TrayManager;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

    void generateAndDisplayPassword();
    void quickGenerateAndCopy();
    void openVaultDialog();

protected:
    void changeEvent(QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private:
    void setupUi();
    void loadVaultAndSettings();
    void saveSettingsToVault();
    bool unlockVaultIfNeeded();
    void updateEntropyDisplay();

    core::PasswordOptions m_options;
    core::VaultDocument m_vaultDoc;
    QString m_vaultPath;
    QByteArray m_masterKey;
    bool m_vaultUnlocked = false;

    // UI elements
    QLineEdit* m_editPassword = nullptr;
    QToolButton* m_btnReveal = nullptr;
    QSlider* m_sliderLength = nullptr;
    QSpinBox* m_spinLength = nullptr;
    QProgressBar* m_entropyBar = nullptr;
    QLabel* m_lblEntropyInfo = nullptr;

    // Action buttons (Icon-only with tooltips)
    QToolButton* m_btnGenerate = nullptr;
    QToolButton* m_btnCopy = nullptr;
    QToolButton* m_btnSaveToVault = nullptr;
    QToolButton* m_btnOpenVault = nullptr;
    QToolButton* m_btnSettings = nullptr;

    TrayManager* m_trayManager = nullptr;
};

} // namespace gui
