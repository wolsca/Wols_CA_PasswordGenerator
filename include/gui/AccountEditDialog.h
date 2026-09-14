#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QToolButton>
#include "core/VaultModel.h"
#include "core/PasswordOptions.h"

namespace gui {

class AccountEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit AccountEditDialog(const core::AccountEntry& account, 
                               const QString& initialPassword,
                               const core::PasswordOptions& genOptions,
                               QWidget* parent = nullptr);

    core::AccountEntry getAccount() const;
    QString getPassword() const;

private:
    void setupUi();

    core::AccountEntry m_account;
    QString m_password;
    core::PasswordOptions m_genOptions;

    QLineEdit* m_editLabel = nullptr;
    QLineEdit* m_editUsername = nullptr;
    QLineEdit* m_editEmail = nullptr;
    QCheckBox* m_chkDefaultEmail = nullptr;
    QLineEdit* m_editPassword = nullptr;
    QToolButton* m_btnReveal = nullptr;
    QToolButton* m_btnGenerate = nullptr;
};

} // namespace gui
