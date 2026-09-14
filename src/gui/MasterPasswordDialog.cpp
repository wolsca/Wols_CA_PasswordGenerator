#include "gui/MasterPasswordDialog.h"
#include "gui/IconUtils.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace gui {

MasterPasswordDialog::MasterPasswordDialog(bool isInitialSetup, QWidget* parent)
    : QDialog(parent), m_isInitialSetup(isInitialSetup)
{
    setWindowTitle(isInitialSetup ? QStringLiteral("Hoofdwachtwoord Instellen") 
                                 : QStringLiteral("Kluis Ontgrendelen"));
    setWindowIcon(IconUtils::getIcon(IconType::Lock, QColor(0, 122, 255)));
    setFixedWidth(420);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    QLabel* infoLabel = new QLabel(this);
    infoLabel->setWordWrap(true);
    if (isInitialSetup) {
        infoLabel->setText(QStringLiteral("Stel een sterk hoofdwachtwoord in voor je kluis. Dit wachtwoord wordt gebruikt om alle opgeslagen wachtwoorden te versleutelen (AES-256-GCM)."));
    } else {
        infoLabel->setText(QStringLiteral("Voer je hoofdwachtwoord in om toegang te krijgen tot de opgeslagen accounts en wachtwoorden."));
    }
    mainLayout->addWidget(infoLabel);

    // Password row
    QLabel* passLabel = new QLabel(QStringLiteral("Hoofdwachtwoord:"), this);
    mainLayout->addWidget(passLabel);

    QHBoxLayout* passLayout = new QHBoxLayout();
    m_passwordEdit = new QLineEdit(this);
    m_passwordEdit->setEchoMode(QLineEdit::Password);
    m_passwordEdit->setPlaceholderText(QStringLiteral("Voer hoofdwachtwoord in"));
    passLayout->addWidget(m_passwordEdit);

    m_revealBtn = new QToolButton(this);
    m_revealBtn->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
    m_revealBtn->setIconSize(QSize(20, 20));
    m_revealBtn->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
    m_revealBtn->setAutoRaise(true);
    passLayout->addWidget(m_revealBtn);

    mainLayout->addLayout(passLayout);

    // Hold to reveal logic
    connect(m_revealBtn, &QToolButton::pressed, this, [this]() {
        m_passwordEdit->setEchoMode(QLineEdit::Normal);
    });
    connect(m_revealBtn, &QToolButton::released, this, [this]() {
        m_passwordEdit->setEchoMode(QLineEdit::Password);
    });

    if (isInitialSetup) {
        QLabel* confLabel = new QLabel(QStringLiteral("Bevestig hoofdwachtwoord:"), this);
        mainLayout->addWidget(confLabel);

        m_confirmEdit = new QLineEdit(this);
        m_confirmEdit->setEchoMode(QLineEdit::Password);
        m_confirmEdit->setPlaceholderText(QStringLiteral("Herhaal hoofdwachtwoord"));
        mainLayout->addWidget(m_confirmEdit);
    }

    m_errorLabel = new QLabel(this);
    m_errorLabel->setStyleSheet("color: #d32f2f; font-weight: bold;");
    m_errorLabel->setVisible(false);
    mainLayout->addWidget(m_errorLabel);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        QString pass = m_passwordEdit->text();
        if (pass.isEmpty()) {
            m_errorLabel->setText(QStringLiteral("Wachtwoord mag niet leeg zijn."));
            m_errorLabel->setVisible(true);
            return;
        }

        if (m_isInitialSetup) {
            if (pass.length() < 6) {
                m_errorLabel->setText(QStringLiteral("Hoofdwachtwoord moet minimaal 6 karakters bevatten."));
                m_errorLabel->setVisible(true);
                return;
            }
            if (m_confirmEdit && pass != m_confirmEdit->text()) {
                m_errorLabel->setText(QStringLiteral("Wachtwoorden komen niet overeen."));
                m_errorLabel->setVisible(true);
                return;
            }
        }

        accept();
    });

    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QString MasterPasswordDialog::getMasterPassword() const {
    return m_passwordEdit->text();
}

} // namespace gui
