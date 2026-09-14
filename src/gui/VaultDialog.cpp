#include "gui/VaultDialog.h"
#include "gui/AccountEditDialog.h"
#include "gui/IconUtils.h"
#include "core/VaultCrypto.h"
#include "core/VaultStorage.h"
#include "core/BiometricAuth.h"
#include "core/ChromeImporter.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QScrollArea>
#include <QFrame>
#include <QInputDialog>
#include <QMessageBox>
#include <QPushButton>
#include <QFileDialog>
#include <QClipboard>
#include <QApplication>
#include <QHeaderView>
#include <QDateTime>
#include <QUuid>
#include <QShortcut>
#include <QKeySequence>

namespace gui {

namespace {
    QString formatTimestamp(const QDateTime& dt) {
        if (!dt.isValid()) return QStringLiteral("Nooit / Never");
        return dt.toLocalTime().toString("yyyy-MM-dd HH:mm:ss");
    }
}

VaultDialog::VaultDialog(const QString& vaultPath, 
                         const QByteArray& masterKey, 
                         const core::VaultDocument& vaultDoc, 
                         QWidget* parent)
    : QDialog(parent), m_vaultPath(vaultPath), m_masterKey(masterKey), m_vaultDoc(vaultDoc)
{
    setWindowTitle(QStringLiteral("Wols Wachtwoordkluis / Password Vault"));
    setWindowIcon(IconUtils::getIcon(IconType::Vault, QColor(0, 122, 255)));
    resize(900, 580);
    setupUi();
    populateTree();
}

void VaultDialog::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSpacing(8);

    // Top Toolbar
    QHBoxLayout* topBar = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText(QStringLiteral("Zoeken in kluis (naam, url, gebruiker, e-mail)... [Ctrl+F]"));
    m_searchEdit->setClearButtonEnabled(true);
    topBar->addWidget(m_searchEdit, 1);

    m_btnAddGroup = new QToolButton(this);
    m_btnAddGroup->setIcon(IconUtils::getIcon(IconType::Plus, QColor(0, 122, 255)));
    m_btnAddGroup->setToolTip(QStringLiteral("Nieuwe groep toevoegen (Ctrl+Shift+N)"));
    topBar->addWidget(m_btnAddGroup);

    m_btnAddItem = new QToolButton(this);
    m_btnAddItem->setIcon(IconUtils::getIcon(IconType::Vault, QColor(0, 122, 255)));
    m_btnAddItem->setToolTip(QStringLiteral("Nieuw kluisitem toevoegen (Ctrl+N)"));
    topBar->addWidget(m_btnAddItem);

    m_btnEditItem = new QToolButton(this);
    m_btnEditItem->setIcon(IconUtils::getIcon(IconType::Edit, QColor(70, 70, 70)));
    m_btnEditItem->setToolTip(QStringLiteral("Geselecteerd item/groep hernoemen (F2)"));
    topBar->addWidget(m_btnEditItem);

    m_btnDeleteItem = new QToolButton(this);
    m_btnDeleteItem->setIcon(IconUtils::getIcon(IconType::Trash, QColor(220, 50, 50)));
    m_btnDeleteItem->setToolTip(QStringLiteral("Geselecteerd item/groep verwijderen (Del)"));
    topBar->addWidget(m_btnDeleteItem);

    m_btnImport = new QToolButton(this);
    m_btnImport->setIcon(IconUtils::getIcon(IconType::Import, QColor(0, 122, 255)));
    m_btnImport->setToolTip(QStringLiteral("Wachtwoorden importeren uit Google Chrome (CSV) [Ctrl+I]"));
    topBar->addWidget(m_btnImport);

    m_btnBackup = new QToolButton(this);
    m_btnBackup->setIcon(IconUtils::getIcon(IconType::Refresh, QColor(70, 70, 70)));
    m_btnBackup->setToolTip(QStringLiteral("Back-up herstellen / Backups beheren"));
    topBar->addWidget(m_btnBackup);

    m_btnSave = new QToolButton(this);
    m_btnSave->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
    m_btnSave->setToolTip(QStringLiteral("Kluis nu opslaan (Ctrl+S)"));
    topBar->addWidget(m_btnSave);

    mainLayout->addLayout(topBar);

    // Keyboard Shortcuts in VaultDialog
    new QShortcut(QKeySequence::Find, this, [this]() { m_searchEdit->setFocus(); m_searchEdit->selectAll(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_N), this, [this]() { m_btnAddGroup->click(); });
    new QShortcut(QKeySequence::New, this, [this]() { m_btnAddItem->click(); });
    new QShortcut(QKeySequence(Qt::Key_F2), this, [this]() { m_btnEditItem->click(); });
    new QShortcut(QKeySequence::Delete, this, [this]() { m_btnDeleteItem->click(); });
    new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_I), this, [this]() { m_btnImport->click(); });
    new QShortcut(QKeySequence::Save, this, [this]() { m_btnSave->click(); });

    // Splitter: Tree (left) and Details (right)
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);

    m_treeWidget = new QTreeWidget(splitter);
    m_treeWidget->setHeaderLabels(QStringList() << QStringLiteral("Kluisstructuur"));
    m_treeWidget->setMinimumWidth(240);
    splitter->addWidget(m_treeWidget);

    // Right details panel
    m_rightPanel = new QWidget(splitter);
    QVBoxLayout* rightLayout = new QVBoxLayout(m_rightPanel);
    rightLayout->setContentsMargins(10, 0, 0, 0);

    // Header info
    QHBoxLayout* itemHeaderLayout = new QHBoxLayout();
    m_itemTitleLabel = new QLabel(QStringLiteral("Selecteer een item"), m_rightPanel);
    m_itemTitleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1a73e8;");
    itemHeaderLayout->addWidget(m_itemTitleLabel, 1);

    m_btnAddAccount = new QToolButton(m_rightPanel);
    m_btnAddAccount->setIcon(IconUtils::getIcon(IconType::Plus, QColor(0, 122, 255)));
    m_btnAddAccount->setToolTip(QStringLiteral("Account toevoegen aan dit item"));
    m_btnAddAccount->setEnabled(false);
    itemHeaderLayout->addWidget(m_btnAddAccount);

    rightLayout->addLayout(itemHeaderLayout);

    m_itemDetailsLabel = new QLabel(m_rightPanel);
    m_itemDetailsLabel->setStyleSheet("color: #666; font-size: 12px;");
    rightLayout->addWidget(m_itemDetailsLabel);

    // Accounts Scroll area
    QScrollArea* scrollArea = new QScrollArea(m_rightPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_accountsContainer = new QWidget();
    m_accountsLayout = new QVBoxLayout(m_accountsContainer);
    m_accountsLayout->setContentsMargins(0, 8, 0, 8);
    m_accountsLayout->setSpacing(10);
    m_accountsLayout->addStretch();

    scrollArea->setWidget(m_accountsContainer);
    rightLayout->addWidget(scrollArea, 1);

    splitter->addWidget(m_rightPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 2);

    mainLayout->addWidget(splitter, 1);

    // Connect signals
    connect(m_searchEdit, &QLineEdit::textChanged, this, [this](const QString& text) {
        populateTree(text);
    });

    connect(m_treeWidget, &QTreeWidget::currentItemChanged, this, [this]() {
        core::VaultItem* item = getSelectedItem();
        m_btnAddAccount->setEnabled(item != nullptr);
        populateAccountList(item);
    });

    connect(m_btnAddGroup, &QToolButton::clicked, this, &VaultDialog::onAddGroup);
    connect(m_btnAddItem, &QToolButton::clicked, this, &VaultDialog::onAddItem);
    connect(m_btnEditItem, &QToolButton::clicked, this, &VaultDialog::onEditItem);
    connect(m_btnDeleteItem, &QToolButton::clicked, this, &VaultDialog::onDeleteItem);
    connect(m_btnAddAccount, &QToolButton::clicked, this, &VaultDialog::onAddAccount);
    connect(m_btnImport, &QToolButton::clicked, this, &VaultDialog::onImportChromeCsv);
    connect(m_btnBackup, &QToolButton::clicked, this, &VaultDialog::onRestoreBackup);
    connect(m_btnSave, &QToolButton::clicked, this, [this]() {
        saveCurrentVault();
        QMessageBox::information(this, QStringLiteral("Kluis Opgeslagen"), 
                                 QStringLiteral("De kluis is succesvol versleuteld en opgeslagen."));
    });
}

core::VaultItem* VaultDialog::getSelectedItem() {
    QTreeWidgetItem* cur = m_treeWidget->currentItem();
    if (!cur) return nullptr;

    QString type = cur->data(0, Qt::UserRole).toString();
    if (type != "item") return nullptr;

    QString grpId = cur->data(0, Qt::UserRole + 1).toString();
    QString itemId = cur->data(0, Qt::UserRole + 2).toString();

    for (auto& grp : m_vaultDoc.groups) {
        if (grp.id == grpId) {
            for (auto& item : grp.items) {
                if (item.id == itemId) {
                    return &item;
                }
            }
        }
    }
    return nullptr;
}

core::VaultGroup* VaultDialog::getSelectedGroup() {
    QTreeWidgetItem* cur = m_treeWidget->currentItem();
    if (!cur) return m_vaultDoc.groups.isEmpty() ? nullptr : &m_vaultDoc.groups.first();

    QString grpId = cur->data(0, Qt::UserRole + 1).toString();
    for (auto& grp : m_vaultDoc.groups) {
        if (grp.id == grpId) {
            return &grp;
        }
    }
    return m_vaultDoc.groups.isEmpty() ? nullptr : &m_vaultDoc.groups.first();
}

void VaultDialog::populateTree(const QString& filter) {
    m_treeWidget->clear();
    QString term = filter.trimmed().toLower();

    for (const auto& grp : m_vaultDoc.groups) {
        QTreeWidgetItem* grpItem = new QTreeWidgetItem(m_treeWidget);
        grpItem->setText(0, grp.name);
        grpItem->setIcon(0, IconUtils::getIcon(IconType::Vault, QColor(0, 122, 255)));
        grpItem->setData(0, Qt::UserRole, "group");
        grpItem->setData(0, Qt::UserRole + 1, grp.id);

        bool groupHasMatches = false;

        for (const auto& item : grp.items) {
            bool matches = term.isEmpty() ||
                           item.title.toLower().contains(term) ||
                           item.url.toLower().contains(term) ||
                           item.category.toLower().contains(term);

            if (!matches) {
                for (const auto& acc : item.accounts) {
                    if (acc.username.toLower().contains(term) ||
                        acc.email.toLower().contains(term) ||
                        acc.label.toLower().contains(term)) {
                        matches = true;
                        break;
                    }
                }
            }

            if (matches) {
                groupHasMatches = true;
                QTreeWidgetItem* child = new QTreeWidgetItem(grpItem);
                child->setText(0, item.title.isEmpty() ? QStringLiteral("(Geen titel)") : item.title);
                child->setIcon(0, IconUtils::getIcon(IconType::Key, QColor(70, 70, 70)));
                child->setData(0, Qt::UserRole, "item");
                child->setData(0, Qt::UserRole + 1, grp.id);
                child->setData(0, Qt::UserRole + 2, item.id);
            }
        }

        if (term.isEmpty() || groupHasMatches) {
            grpItem->setExpanded(true);
        } else {
            delete grpItem;
        }
    }
}

void VaultDialog::populateAccountList(core::VaultItem* item) {
    // Clear old accounts widgets
    QLayoutItem* child;
    while ((child = m_accountsLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            delete child->widget();
        }
        delete child;
    }

    if (!item) {
        m_itemTitleLabel->setText(QStringLiteral("Selecteer een item"));
        m_itemDetailsLabel->setText(QString());
        m_accountsLayout->addStretch();
        return;
    }

    m_itemTitleLabel->setText(item->title.isEmpty() ? QStringLiteral("(Geen titel)") : item->title);
    QString details = QString("URL: %1 | Categorie: %2\nAangemaakt: %3 | Gewijzigd: %4 | Laatst geopend: %5")
                      .arg(item->url.isEmpty() ? "-" : item->url,
                           item->category.isEmpty() ? "-" : item->category,
                           formatTimestamp(item->createdAt),
                           formatTimestamp(item->updatedAt),
                           formatTimestamp(item->lastAccessed));
    m_itemDetailsLabel->setText(details);

    if (item->accounts.isEmpty()) {
        QLabel* emptyLabel = new QLabel(QStringLiteral("Nog geen accounts opgeslagen onder dit item. Klik op '+' om een account toe te voegen."), m_accountsContainer);
        emptyLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
        m_accountsLayout->addWidget(emptyLabel);
    } else {
        for (int i = 0; i < item->accounts.size(); ++i) {
            const auto& acc = item->accounts[i];
            int accIdx = i;

            QFrame* card = new QFrame(m_accountsContainer);
            card->setFrameShape(QFrame::StyledPanel);
            card->setStyleSheet("QFrame { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 6px; padding: 6px; }");

            QVBoxLayout* cardLayout = new QVBoxLayout(card);
            cardLayout->setSpacing(6);

            // Row 1: Label + Default badge + Actions
            QHBoxLayout* headerRow = new QHBoxLayout();
            QLabel* labelText = new QLabel(acc.label.isEmpty() ? QString("Account %1").arg(i + 1) : acc.label, card);
            labelText->setStyleSheet("font-weight: bold; font-size: 13px;");
            headerRow->addWidget(labelText);

            if (acc.isDefaultEmail) {
                QLabel* badge = new QLabel(QStringLiteral("★ Default"), card);
                badge->setStyleSheet("background-color: #e3f2fd; color: #1976d2; font-weight: bold; padding: 2px 6px; border-radius: 4px; font-size: 11px;");
                headerRow->addWidget(badge);
            }

            headerRow->addStretch();

            // Edit Account
            QToolButton* btnEditAcc = new QToolButton(card);
            btnEditAcc->setIcon(IconUtils::getIcon(IconType::Edit, QColor(70, 70, 70)));
            btnEditAcc->setToolTip(QStringLiteral("Account bewerken"));
            btnEditAcc->setAutoRaise(true);
            headerRow->addWidget(btnEditAcc);

            // Delete Account
            QToolButton* btnDelAcc = new QToolButton(card);
            btnDelAcc->setIcon(IconUtils::getIcon(IconType::Trash, QColor(220, 50, 50)));
            btnDelAcc->setToolTip(QStringLiteral("Account verwijderen"));
            btnDelAcc->setAutoRaise(true);
            headerRow->addWidget(btnDelAcc);

            cardLayout->addLayout(headerRow);

            // Row 2: Username & Email with quick copy
            if (!acc.username.isEmpty()) {
                QHBoxLayout* userRow = new QHBoxLayout();
                QLabel* uIcon = new QLabel(card);
                uIcon->setPixmap(IconUtils::getPixmap(IconType::User, QColor(100, 100, 100), 16));
                QLabel* uText = new QLabel(acc.username, card);
                userRow->addWidget(uIcon);
                userRow->addWidget(uText);
                userRow->addStretch();

                QToolButton* btnCopyUser = new QToolButton(card);
                btnCopyUser->setIcon(IconUtils::getIcon(IconType::Copy, QColor(70, 70, 70)));
                btnCopyUser->setToolTip(QStringLiteral("Gebruikersnaam kopiëren"));
                btnCopyUser->setAutoRaise(true);
                connect(btnCopyUser, &QToolButton::clicked, this, [acc]() {
                    QApplication::clipboard()->setText(acc.username);
                });
                userRow->addWidget(btnCopyUser);
                cardLayout->addLayout(userRow);
            }

            if (!acc.email.isEmpty()) {
                QHBoxLayout* emailRow = new QHBoxLayout();
                QLabel* eIcon = new QLabel(card);
                eIcon->setPixmap(IconUtils::getPixmap(IconType::Mail, QColor(100, 100, 100), 16));
                QLabel* eText = new QLabel(acc.email, card);
                emailRow->addWidget(eIcon);
                emailRow->addWidget(eText);
                emailRow->addStretch();

                QToolButton* btnCopyEmail = new QToolButton(card);
                btnCopyEmail->setIcon(IconUtils::getIcon(IconType::Copy, QColor(70, 70, 70)));
                btnCopyEmail->setToolTip(QStringLiteral("E-mailadres kopiëren"));
                btnCopyEmail->setAutoRaise(true);
                connect(btnCopyEmail, &QToolButton::clicked, this, [acc]() {
                    QApplication::clipboard()->setText(acc.email);
                });
                emailRow->addWidget(btnCopyEmail);
                cardLayout->addLayout(emailRow);
            }

            // Row 3: Password + Hold to Reveal + Copy
            QHBoxLayout* passRow = new QHBoxLayout();
            QLabel* pIcon = new QLabel(card);
            pIcon->setPixmap(IconUtils::getPixmap(IconType::Lock, QColor(100, 100, 100), 16));
            passRow->addWidget(pIcon);

            QLineEdit* passEdit = new QLineEdit(card);
            passEdit->setEchoMode(QLineEdit::Password);
            passEdit->setReadOnly(true);
            passEdit->setText("••••••••••••");
            passRow->addWidget(passEdit, 1);

            QToolButton* btnReveal = new QToolButton(card);
            btnReveal->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
            btnReveal->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
            btnReveal->setAutoRaise(true);
            passRow->addWidget(btnReveal);

            QToolButton* btnCopyPass = new QToolButton(card);
            btnCopyPass->setIcon(IconUtils::getIcon(IconType::Copy, QColor(0, 122, 255)));
            btnCopyPass->setToolTip(QStringLiteral("Wachtwoord kopiëren"));
            btnCopyPass->setAutoRaise(true);
            passRow->addWidget(btnCopyPass);

            cardLayout->addLayout(passRow);

            // Row 4: Last accessed timestamp
            QLabel* accessLabel = new QLabel(QString("Laatst geopend: %1").arg(formatTimestamp(acc.lastAccessed)), card);
            accessLabel->setStyleSheet("color: #777; font-size: 11px;");
            cardLayout->addWidget(accessLabel);

            // Decrypt password helper
            auto decryptHelper = [this, acc, item, accIdx]() -> QString {
                QString plaintext;
                if (!core::VaultCrypto::decryptPassword(acc.encryptedPassword, acc.nonce, acc.authTag, m_masterKey, plaintext)) {
                    return QString();
                }
                // Update last accessed
                QDateTime now = QDateTime::currentDateTimeUtc();
                if (item && accIdx < item->accounts.size()) {
                    item->accounts[accIdx].lastAccessed = now;
                    item->lastAccessed = now;
                    m_modified = true;
                    saveCurrentVault();
                }
                return plaintext;
            };

            // Connect Hold to reveal
            connect(btnReveal, &QToolButton::pressed, this, [this, passEdit, decryptHelper, accessLabel]() {
                if (m_vaultDoc.settings.requireBiometricsForReveal) {
                    if (!core::BiometricAuth::authenticate(QStringLiteral("Verifieer uw identiteit om het opgeslagen kluiswachtwoord te bekijken."), this)) {
                        return;
                    }
                }
                QString pt = decryptHelper();
                if (!pt.isEmpty()) {
                    passEdit->setText(pt);
                    passEdit->setEchoMode(QLineEdit::Normal);
                    accessLabel->setText(QString("Laatst geopend: %1").arg(formatTimestamp(QDateTime::currentDateTimeUtc())));
                }
            });
            connect(btnReveal, &QToolButton::released, this, [passEdit]() {
                passEdit->setText("••••••••••••");
                passEdit->setEchoMode(QLineEdit::Password);
            });

            // Connect Copy Password
            connect(btnCopyPass, &QToolButton::clicked, this, [this, decryptHelper, accessLabel]() {
                if (m_vaultDoc.settings.requireBiometricsForCopy) {
                    if (!core::BiometricAuth::authenticate(QStringLiteral("Verifieer uw identiteit om het opgeslagen kluiswachtwoord te kopiëren."), this)) {
                        return;
                    }
                }
                QString pt = decryptHelper();
                if (!pt.isEmpty()) {
                    QApplication::clipboard()->setText(pt);
                    accessLabel->setText(QString("Laatst geopend: %1").arg(formatTimestamp(QDateTime::currentDateTimeUtc())));
                    QMessageBox::information(this, QStringLiteral("Gekopieerd"), QStringLiteral("Wachtwoord gekopieerd naar klembord."));
                } else {
                    QMessageBox::warning(this, QStringLiteral("Fout"), QStringLiteral("Kan wachtwoord niet ontsleutelen met huidige sleutel."));
                }
            });

            connect(btnEditAcc, &QToolButton::clicked, this, [this, accIdx]() {
                onEditAccount(accIdx);
            });
            connect(btnDelAcc, &QToolButton::clicked, this, [this, accIdx]() {
                onDeleteAccount(accIdx);
            });

            m_accountsLayout->addWidget(card);
        }
    }

    m_accountsLayout->addStretch();
}

void VaultDialog::onAddGroup() {
    bool ok = false;
    QString name = QInputDialog::getText(this, QStringLiteral("Nieuwe Groep"), 
                                        QStringLiteral("Voer naam van de groep in:"), 
                                        QLineEdit::Normal, QString(), &ok);
    if (ok && !name.trimmed().isEmpty()) {
        core::VaultGroup grp;
        grp.id = "grp_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        grp.name = name.trimmed();
        m_vaultDoc.groups.append(grp);
        m_modified = true;
        saveCurrentVault();
        populateTree(m_searchEdit->text());
    }
}

void VaultDialog::onAddItem() {
    core::VaultGroup* grp = getSelectedGroup();
    if (!grp) {
        onAddGroup();
        grp = getSelectedGroup();
        if (!grp) return;
    }

    bool ok = false;
    QString title = QInputDialog::getText(this, QStringLiteral("Nieuw Kluisitem"), 
                                         QString("Voer item titel in voor groep '%1':").arg(grp->name), 
                                         QLineEdit::Normal, QString(), &ok);
    if (ok && !title.trimmed().isEmpty()) {
        core::VaultItem item;
        item.id = "item_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
        item.title = title.trimmed();
        item.createdAt = QDateTime::currentDateTimeUtc();
        item.updatedAt = item.createdAt;
        item.lastAccessed = item.createdAt;
        grp->items.append(item);
        m_modified = true;
        saveCurrentVault();
        populateTree(m_searchEdit->text());
    }
}

void VaultDialog::onEditItem() {
    core::VaultItem* item = getSelectedItem();
    if (item) {
        bool ok = false;
        QString title = QInputDialog::getText(this, QStringLiteral("Item Bewerken"), 
                                             QStringLiteral("Titel:"), 
                                             QLineEdit::Normal, item->title, &ok);
        if (ok && !title.trimmed().isEmpty()) {
            item->title = title.trimmed();
            item->updatedAt = QDateTime::currentDateTimeUtc();
            m_modified = true;
            saveCurrentVault();
            populateTree(m_searchEdit->text());
            populateAccountList(item);
        }
        return;
    }

    core::VaultGroup* grp = getSelectedGroup();
    if (grp) {
        bool ok = false;
        QString name = QInputDialog::getText(this, QStringLiteral("Groep Bewerken"), 
                                            QStringLiteral("Groepsnaam:"), 
                                            QLineEdit::Normal, grp->name, &ok);
        if (ok && !name.trimmed().isEmpty()) {
            grp->name = name.trimmed();
            m_modified = true;
            saveCurrentVault();
            populateTree(m_searchEdit->text());
        }
    }
}

void VaultDialog::onDeleteItem() {
    core::VaultItem* item = getSelectedItem();
    if (item) {
        auto res = QMessageBox::question(this, QStringLiteral("Item Verwijderen"), 
                                         QString("Weet je zeker dat je item '%1' en alle bijbehorende accounts wilt verwijderen?").arg(item->title));
        if (res == QMessageBox::Yes) {
            for (auto& grp : m_vaultDoc.groups) {
                for (int i = 0; i < grp.items.size(); ++i) {
                    if (grp.items[i].id == item->id) {
                        grp.items.removeAt(i);
                        m_modified = true;
                        saveCurrentVault();
                        populateTree(m_searchEdit->text());
                        populateAccountList(nullptr);
                        return;
                    }
                }
            }
        }
        return;
    }

    core::VaultGroup* grp = getSelectedGroup();
    if (grp) {
        auto res = QMessageBox::question(this, QStringLiteral("Groep Verwijderen"), 
                                         QString("Weet je zeker dat je groep '%1' en alle items wilt verwijderen?").arg(grp->name));
        if (res == QMessageBox::Yes) {
            for (int i = 0; i < m_vaultDoc.groups.size(); ++i) {
                if (m_vaultDoc.groups[i].id == grp->id) {
                    m_vaultDoc.groups.removeAt(i);
                    m_modified = true;
                    saveCurrentVault();
                    populateTree(m_searchEdit->text());
                    populateAccountList(nullptr);
                    return;
                }
            }
        }
    }
}

void VaultDialog::onAddAccount() {
    core::VaultItem* item = getSelectedItem();
    if (!item) return;

    core::AccountEntry newAcc;
    newAcc.id = "acc_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    newAcc.lastAccessed = QDateTime::currentDateTimeUtc();
    newAcc.isDefaultEmail = item->accounts.isEmpty(); // First account default by default

    AccountEditDialog dlg(newAcc, QString(), m_vaultDoc.settings.toPasswordOptions(), this);
    if (dlg.exec() == QDialog::Accepted) {
        core::AccountEntry created = dlg.getAccount();
        QString plainPass = dlg.getPassword();

        if (!plainPass.isEmpty()) {
            core::VaultCrypto::encryptPassword(plainPass, m_masterKey, 
                                               created.encryptedPassword, 
                                               created.nonce, 
                                               created.authTag);
        }

        // If this account is default, unmark others
        if (created.isDefaultEmail) {
            for (auto& acc : item->accounts) {
                acc.isDefaultEmail = false;
            }
        }

        created.lastAccessed = QDateTime::currentDateTimeUtc();
        item->accounts.append(created);
        item->updatedAt = QDateTime::currentDateTimeUtc();
        item->lastAccessed = item->updatedAt;

        m_modified = true;
        saveCurrentVault();
        populateAccountList(item);
    }
}

void VaultDialog::onEditAccount(int accountIndex) {
    core::VaultItem* item = getSelectedItem();
    if (!item || accountIndex < 0 || accountIndex >= item->accounts.size()) return;

    core::AccountEntry& acc = item->accounts[accountIndex];
    QString currentPass;
    core::VaultCrypto::decryptPassword(acc.encryptedPassword, acc.nonce, acc.authTag, m_masterKey, currentPass);

    AccountEditDialog dlg(acc, currentPass, m_vaultDoc.settings.toPasswordOptions(), this);
    if (dlg.exec() == QDialog::Accepted) {
        core::AccountEntry updated = dlg.getAccount();
        QString plainPass = dlg.getPassword();

        if (!plainPass.isEmpty()) {
            core::VaultCrypto::encryptPassword(plainPass, m_masterKey, 
                                               updated.encryptedPassword, 
                                               updated.nonce, 
                                               updated.authTag);
        }

        // If this account is default, unmark others
        if (updated.isDefaultEmail) {
            for (int i = 0; i < item->accounts.size(); ++i) {
                if (i != accountIndex) {
                    item->accounts[i].isDefaultEmail = false;
                }
            }
        }

        updated.lastAccessed = QDateTime::currentDateTimeUtc();
        item->accounts[accountIndex] = updated;
        item->updatedAt = QDateTime::currentDateTimeUtc();
        item->lastAccessed = item->updatedAt;

        m_modified = true;
        saveCurrentVault();
        populateAccountList(item);
    }
}

void VaultDialog::onDeleteAccount(int accountIndex) {
    core::VaultItem* item = getSelectedItem();
    if (!item || accountIndex < 0 || accountIndex >= item->accounts.size()) return;

    auto res = QMessageBox::question(this, QStringLiteral("Account Verwijderen"), 
                                     QStringLiteral("Weet je zeker dat je dit account wilt verwijderen?"));
    if (res == QMessageBox::Yes) {
        item->accounts.removeAt(accountIndex);
        item->updatedAt = QDateTime::currentDateTimeUtc();
        m_modified = true;
        saveCurrentVault();
        populateAccountList(item);
    }
}

void VaultDialog::onRestoreBackup() {
    QStringList backups = core::VaultStorage::listBackups();
    if (backups.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Backups"), 
                                 QStringLiteral("Er zijn momenteel nog geen lokale back-up snapshots aangemaakt."));
        return;
    }

    bool ok = false;
    QString selected = QInputDialog::getItem(this, QStringLiteral("Back-up Herstellen"), 
                                            QStringLiteral("Kies een back-up moment om te herstellen:"), 
                                            backups, 0, false, &ok);
    if (ok && !selected.isEmpty()) {
        QString err;
        if (core::VaultStorage::restoreBackup(selected, m_vaultPath, &err)) {
            m_vaultDoc = core::VaultStorage::loadVault(m_vaultPath);
            populateTree(m_searchEdit->text());
            populateAccountList(nullptr);
            QMessageBox::information(this, QStringLiteral("Hersteld"), 
                                     QString("Back-up '%1' is succesvol hersteld.").arg(selected));
        } else {
            QMessageBox::warning(this, QStringLiteral("Fout"), 
                                 QString("Kan back-up niet herstellen: %1").arg(err));
        }
    }
}

void VaultDialog::onImportChromeCsv() {
    QString filePath = QFileDialog::getOpenFileName(
        this, 
        QStringLiteral("Google Chrome CSV-exportbestand Selecteren"), 
        QString(), 
        QStringLiteral("CSV-bestanden (*.csv);;Alle bestanden (*.*)")
    );

    if (filePath.isEmpty()) {
        return;
    }

    core::ChromeImportResult result = core::ChromeImporter::importFromCsvFile(filePath, m_masterKey, m_vaultDoc);

    if (!result.success) {
        QString errMsg = result.errors.isEmpty() ? QStringLiteral("Geen geldige accounts of wachtwoorden gevonden in het CSV-bestand.") : result.errors.join(QLatin1Char('\n'));
        QMessageBox::warning(this, QStringLiteral("Import Mislukt"), QString("Kon wachtwoorden niet importeren:\n%1").arg(errMsg));
        return;
    }

    m_modified = true;
    saveCurrentVault();
    populateTree(m_searchEdit->text());
    populateAccountList(nullptr);

    // Toon succesmelding
    QString summaryMsg = QString("Import succesvol voltooid!\n\n"
                                 "• Nieuwe accounts geïmporteerd: %1\n"
                                 "• Bestaande accounts bijgewerkt: %2")
        .arg(result.importedCount)
        .arg(result.updatedCount);

    QMessageBox::information(this, QStringLiteral("Import Voltooid"), summaryMsg);

    // Vraag aan de gebruiker of het CSV-bestand leeggemaakt mag worden
    QMessageBox msgBox(this);
    msgBox.setWindowTitle(QStringLiteral("Bronbestand Beveiligen"));
    msgBox.setText(QString("Beveiligingswaarschuwing:\n"
                           "Het geïmporteerde CSV-bestand bevat al je wachtwoorden in onbeveiligde platte tekst.\n\n"
                           "Wil je het bronbestand '%1' nu veilig leegmaken (overschrijven met random data en leegmaken naar 0 bytes) in plaats van het zo te laten staan?")
                   .arg(QFileInfo(filePath).fileName()));
    msgBox.setIcon(QMessageBox::Question);
    QPushButton* wipeBtn = msgBox.addButton(QStringLiteral("Bestand Leegmaken"), QMessageBox::AcceptRole);
    QPushButton* keepBtn = msgBox.addButton(QStringLiteral("Bestand Behouden"), QMessageBox::RejectRole);
    msgBox.setDefaultButton(wipeBtn);

    msgBox.exec();

    if (msgBox.clickedButton() == wipeBtn) {
        QString wipeErr;
        if (core::ChromeImporter::wipeFile(filePath, &wipeErr)) {
            QMessageBox::information(this, QStringLiteral("Bestand Leeggemaakt"), 
                QStringLiteral("Het CSV-bestand is succesvol overschreven en leeggemaakt (0 bytes)."));
        } else {
            QMessageBox::warning(this, QStringLiteral("Fout bij Leegmaken"), 
                QString("Kon het bestand niet leegmaken: %1").arg(wipeErr));
        }
    }
}

void VaultDialog::saveCurrentVault() {
    m_vaultDoc.lastSynced = QDateTime::currentDateTimeUtc();
    QString err;
    core::VaultStorage::saveVault(m_vaultPath, m_vaultDoc, &err);
}

core::VaultDocument VaultDialog::getVaultDocument() const {
    return m_vaultDoc;
}

bool VaultDialog::hasModifications() const {
    return m_modified;
}

} // namespace gui
