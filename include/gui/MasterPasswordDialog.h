#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QToolButton>
#include <QLabel>
#include <QPushButton>

namespace gui {

class MasterPasswordDialog : public QDialog {
    Q_OBJECT
public:
    explicit MasterPasswordDialog(bool isInitialSetup = false, QWidget* parent = nullptr);

    QString getMasterPassword() const;

private:
    QLineEdit* m_passwordEdit = nullptr;
    QLineEdit* m_confirmEdit = nullptr;
    QToolButton* m_revealBtn = nullptr;
    QLabel* m_errorLabel = nullptr;
    bool m_isInitialSetup = false;
};

} // namespace gui
