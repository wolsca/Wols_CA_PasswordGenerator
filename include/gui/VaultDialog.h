#pragma once

#include <QDialog>
#include <QTreeWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QSplitter>
#include <QVBoxLayout>
#include "core/VaultModel.h"

namespace gui {

class VaultDialog : public QDialog {
    Q_OBJECT
public:
    explicit VaultDialog(const QString& vaultPath, 
                         const QByteArray& masterKey, 
                         const core::VaultDocument& vaultDoc, 
                         QWidget* parent = nullptr);

    core::VaultDocument getVaultDocument() const;
    bool hasModifications() const;

private:
    void setupUi();
    void populateTree(const QString& filter = QString());
    void populateAccountList(core::VaultItem* item);
    void saveCurrentVault();

    // Group / Item / Account actions
    void onAddGroup();
    void onAddItem();
    void onEditItem();
    void onDeleteItem();
    void onAddAccount();
    void onEditAccount(int accountIndex);
    void onDeleteAccount(int accountIndex);
    void onRestoreBackup();
    void onImportChromeCsv();

    core::VaultItem* getSelectedItem();
    core::VaultGroup* getSelectedGroup();

    QString m_vaultPath;
    QByteArray m_masterKey;
    core::VaultDocument m_vaultDoc;
    bool m_modified = false;

    // UI elements
    QLineEdit* m_searchEdit = nullptr;
    QTreeWidget* m_treeWidget = nullptr;
    QWidget* m_rightPanel = nullptr;
    QLabel* m_itemTitleLabel = nullptr;
    QLabel* m_itemDetailsLabel = nullptr;
    QVBoxLayout* m_accountsLayout = nullptr;
    QWidget* m_accountsContainer = nullptr;

    QToolButton* m_btnAddGroup = nullptr;
    QToolButton* m_btnAddItem = nullptr;
    QToolButton* m_btnEditItem = nullptr;
    QToolButton* m_btnDeleteItem = nullptr;
    QToolButton* m_btnAddAccount = nullptr;
    QToolButton* m_btnBackup = nullptr;
    QToolButton* m_btnImport = nullptr;
    QToolButton* m_btnSave = nullptr;
};

} // namespace gui
