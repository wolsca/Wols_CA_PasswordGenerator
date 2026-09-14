#include "gui/AccountEditDialog.h"
#include "gui/IconUtils.h"
#include "core/PasswordGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QMessageBox>

namespace gui {

AccountEditDialog::AccountEditDialog(const core::AccountEntry& account, 
                                     const QString& initialPassword,
                                     const core::PasswordOptions& genOptions,
                                     QWidget* parent)
    : QDialog(parent), m_account(account), m_password(initialPassword), m_genOptions(genOptions)
{
    setWindowTitle(m_account.id.isEmpty() ? QStringLiteral("Account Toevoegen") 
                                         : QStringLiteral("Account Bewerken"));
    setWindowIcon(IconUtils::getIcon(IconType::User, QColor(0, 122, 255)));
    setFixedWidth(460);
    setupUi();
}

void AccountEditDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(14);

    QFormLayout* form = new QFormLayout();
    form->setLabelAlignment(Qt::AlignLeft);
    form->setSpacing(10);

    m_editLabel = new QLineEdit(this);
    m_editLabel->setPlaceholderText(QStringLiteral("Bijv. Privé, Werk, Hoofdaccount"));
    m_editLabel->setText(m_account.label);
    form->addRow(QStringLiteral("Label / Naam:"), m_editLabel);

    m_editUsername = new QLineEdit(this);
    m_editUsername->setPlaceholderText(QStringLiteral("Gebruikersnaam"));
    m_editUsername->setText(m_account.username);
    form->addRow(QStringLiteral("Gebruikersnaam:"), m_editUsername);

    m_editEmail = new QLineEdit(this);
    m_editEmail->setPlaceholderText(QStringLiteral("naam@voorbeeld.nl"));
    m_editEmail->setText(m_account.email);
    form->addRow(QStringLiteral("E-mailadres:"), m_editEmail);

    m_chkDefaultEmail = new QCheckBox(QStringLiteral("Standaard e-mailadres (Default)"), this);
    m_chkDefaultEmail->setToolTip(QStringLiteral("Vink aan als dit het standaard e-mailadres is voor automatisch invullen"));
    m_chkDefaultEmail->setChecked(m_account.isDefaultEmail);
    form->addRow(QStringLiteral(""), m_chkDefaultEmail);

    // Password row
    QHBoxLayout* passLayout = new QHBoxLayout();
    m_editPassword = new QLineEdit(this);
    m_editPassword->setEchoMode(QLineEdit::Password);
    m_editPassword->setPlaceholderText(QStringLiteral("Wachtwoord"));
    m_editPassword->setText(m_password);
    passLayout->addWidget(m_editPassword);

    m_btnReveal = new QToolButton(this);
    m_btnReveal->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
    m_btnReveal->setIconSize(QSize(20, 20));
    m_btnReveal->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
    m_btnReveal->setAutoRaise(true);
    passLayout->addWidget(m_btnReveal);

    m_btnGenerate = new QToolButton(this);
    m_btnGenerate->setIcon(IconUtils::getIcon(IconType::Refresh, QColor(0, 122, 255)));
    m_btnGenerate->setIconSize(QSize(20, 20));
    m_btnGenerate->setToolTip(QStringLiteral("Genereer nieuw wachtwoord"));
    m_btnGenerate->setAutoRaise(true);
    passLayout->addWidget(m_btnGenerate);

    form->addRow(QStringLiteral("Wachtwoord:"), passLayout);

    mainLayout->addLayout(form);

    // Hold to reveal logic
    connect(m_btnReveal, &QToolButton::pressed, this, [this]() {
        m_editPassword->setEchoMode(QLineEdit::Normal);
    });
    connect(m_btnReveal, &QToolButton::released, this, [this]() {
        m_editPassword->setEchoMode(QLineEdit::Password);
    });

    // Generate password
    connect(m_btnGenerate, &QToolButton::clicked, this, [this]() {
        std::string generated = core::PasswordGenerator::generate(m_genOptions);
        m_editPassword->setText(QString::fromStdString(generated));
    });

    // Button box
    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        if (m_editLabel->text().trimmed().isEmpty() && 
            m_editUsername->text().trimmed().isEmpty() && 
            m_editEmail->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, QStringLiteral("Invoer vereist"), 
                                 QStringLiteral("Voer minimaal een label, gebruikersnaam of e-mailadres in."));
            return;
        }
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

core::AccountEntry AccountEditDialog::getAccount() const {
    core::AccountEntry acc = m_account;
    acc.label = m_editLabel->text().trimmed();
    acc.username = m_editUsername->text().trimmed();
    acc.email = m_editEmail->text().trimmed();
    acc.isDefaultEmail = m_chkDefaultEmail->isChecked();
    return acc;
}

QString AccountEditDialog::getPassword() const {
    return m_editPassword->text();
}

} // namespace gui
