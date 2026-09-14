#include "gui/VaultDialog.h"
#include "gui/AccountEditDialog.h"
#include "gui/IconUtils.h"
#include "core/VaultCrypto.h"
#include "core/VaultStorage.h"
#include "core/BiometricAuth.h"
#include "core/ChromeImporter.h"
#include "core/PasswordGenerator.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QCheckBox>
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
    rightLayout->setSpacing(8);

    // Item Header Container (dynamic view or edit mode)
    m_itemHeaderContainer = new QWidget(m_rightPanel);
    m_itemHeaderLayout = new QVBoxLayout(m_itemHeaderContainer);
    m_itemHeaderLayout->setContentsMargins(0, 0, 0, 0);
    m_itemHeaderLayout->setSpacing(6);
    rightLayout->addWidget(m_itemHeaderContainer);

    // Accounts Scroll area
    QScrollArea* scrollArea = new QScrollArea(m_rightPanel);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);

    m_accountsContainer = new QWidget();
    m_accountsLayout = new QVBoxLayout(m_accountsContainer);
    m_accountsLayout->setContentsMargins(0, 4, 0, 4);
    m_accountsLayout->setSpacing(10);

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
        m_editingAccountIndex = -1;
        m_editingItem = false;
        core::VaultItem* item = getSelectedItem();
        populateAccountList(item);
    });

    connect(m_btnAddGroup, &QToolButton::clicked, this, &VaultDialog::onAddGroup);
    connect(m_btnAddItem, &QToolButton::clicked, this, &VaultDialog::onAddItem);
    connect(m_btnEditItem, &QToolButton::clicked, this, &VaultDialog::onEditItem);
    connect(m_btnDeleteItem, &QToolButton::clicked, this, &VaultDialog::onDeleteItem);
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
    // 1. Clear item header layout
    QLayoutItem* hChild;
    while ((hChild = m_itemHeaderLayout->takeAt(0)) != nullptr) {
        if (hChild->widget()) delete hChild->widget();
        delete hChild;
    }

    // 2. Clear accounts layout
    QLayoutItem* aChild;
    while ((aChild = m_accountsLayout->takeAt(0)) != nullptr) {
        if (aChild->widget()) delete aChild->widget();
        delete aChild;
    }

    if (!item) {
        QLabel* emptyMsg = new QLabel(QStringLiteral("Selecteer een item uit de kluisstructuur links."), m_itemHeaderContainer);
        emptyMsg->setStyleSheet("font-size: 14px; color: #888; padding: 10px;");
        m_itemHeaderLayout->addWidget(emptyMsg);
        m_accountsLayout->addStretch();
        return;
    }

    // --- ITEM HEADER SECTION ---
    if (!m_editingItem) {
        // VIEW MODE: Show only filled item fields
        QFrame* itemCard = new QFrame(m_itemHeaderContainer);
        itemCard->setStyleSheet("QFrame { background-color: #f1f5f9; border: 1px solid #cbd5e1; border-radius: 6px; padding: 8px; }");
        QVBoxLayout* icLayout = new QVBoxLayout(itemCard);
        icLayout->setSpacing(6);

        // Header Row: Title + Category + Edit button + Add Account button
        QHBoxLayout* titleRow = new QHBoxLayout();
        QLabel* titleLabel = new QLabel(item->title.isEmpty() ? QStringLiteral("(Geen titel)") : item->title, itemCard);
        titleLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #1e293b;");
        titleRow->addWidget(titleLabel);

        if (!item->category.isEmpty()) {
            QLabel* catBadge = new QLabel(item->category, itemCard);
            catBadge->setStyleSheet("background-color: #e0f2fe; color: #0284c7; font-weight: bold; padding: 2px 8px; border-radius: 4px; font-size: 11px;");
            titleRow->addWidget(catBadge);
        }

        titleRow->addStretch();

        QToolButton* btnEditItemHeader = new QToolButton(itemCard);
        btnEditItemHeader->setIcon(IconUtils::getIcon(IconType::Edit, QColor(70, 70, 70)));
        btnEditItemHeader->setToolTip(QStringLiteral("Item bewerken (Titel, URL, Categorie, Notities) [F2]"));
        btnEditItemHeader->setAutoRaise(true);
        connect(btnEditItemHeader, &QToolButton::clicked, this, [this, item]() {
            m_editingItem = true;
            populateAccountList(item);
        });
        titleRow->addWidget(btnEditItemHeader);

        m_btnAddAccount = new QToolButton(itemCard);
        m_btnAddAccount->setIcon(IconUtils::getIcon(IconType::Plus, QColor(0, 122, 255)));
        m_btnAddAccount->setToolTip(QStringLiteral("Account toevoegen aan dit item"));
        m_btnAddAccount->setAutoRaise(true);
        connect(m_btnAddAccount, &QToolButton::clicked, this, &VaultDialog::onAddAccount);
        titleRow->addWidget(m_btnAddAccount);

        icLayout->addLayout(titleRow);

        // Only filled item fields:
        if (!item->url.isEmpty()) {
            QHBoxLayout* urlRow = new QHBoxLayout();
            QLabel* urlIcon = new QLabel(itemCard);
            urlIcon->setPixmap(IconUtils::getPixmap(IconType::Key, QColor(100, 100, 100), 16));
            QLabel* urlLabel = new QLabel(QString("<b>URL:</b> <a href=\"%1\" style=\"color: #0284c7; text-decoration: none;\">%1</a>").arg(item->url), itemCard);
            urlLabel->setOpenExternalLinks(true);
            urlRow->addWidget(urlIcon);
            urlRow->addWidget(urlLabel);
            urlRow->addStretch();

            QToolButton* btnCopyUrl = new QToolButton(itemCard);
            btnCopyUrl->setIcon(IconUtils::getIcon(IconType::Copy, QColor(70, 70, 70)));
            btnCopyUrl->setToolTip(QStringLiteral("URL kopiëren"));
            btnCopyUrl->setAutoRaise(true);
            connect(btnCopyUrl, &QToolButton::clicked, this, [item]() {
                QApplication::clipboard()->setText(item->url);
            });
            urlRow->addWidget(btnCopyUrl);
            icLayout->addLayout(urlRow);
        }

        // Notes (decrypted if present)
        if (!item->notesEncrypted.isEmpty()) {
            QString decryptedNotes;
            if (core::VaultCrypto::decryptPassword(item->notesEncrypted, item->notesNonce, item->notesTag, m_masterKey, decryptedNotes) && !decryptedNotes.isEmpty()) {
                QLabel* noteLabel = new QLabel(QString("<b>Notities:</b> %1").arg(decryptedNotes.toHtmlEscaped()), itemCard);
                noteLabel->setWordWrap(true);
                noteLabel->setStyleSheet("color: #475569; font-size: 12px; background: #ffffff; padding: 4px 8px; border-radius: 4px; border: 1px solid #e2e8f0;");
                icLayout->addWidget(noteLabel);
            }
        }

        // Timestamps
        QLabel* metaLabel = new QLabel(QString("Aangemaakt: %1 | Gewijzigd: %2 | Laatst geopend: %3")
            .arg(formatTimestamp(item->createdAt), formatTimestamp(item->updatedAt), formatTimestamp(item->lastAccessed)), itemCard);
        metaLabel->setStyleSheet("color: #94a3b8; font-size: 11px;");
        icLayout->addWidget(metaLabel);

        m_itemHeaderLayout->addWidget(itemCard);
    } else {
        // EDIT MODE for Item: Show ALL editable fields with Cancel and Save icon buttons
        QFrame* editFrame = new QFrame(m_itemHeaderContainer);
        editFrame->setStyleSheet("QFrame { background-color: #f0f7ff; border: 1.5px solid #007aff; border-radius: 6px; padding: 10px; }");
        QVBoxLayout* efLayout = new QVBoxLayout(editFrame);
        efLayout->setSpacing(8);

        QHBoxLayout* efHeader = new QHBoxLayout();
        QLabel* editTitle = new QLabel(QStringLiteral("Item Gegevens Bewerken"), editFrame);
        editTitle->setStyleSheet("font-weight: bold; font-size: 14px; color: #007aff;");
        efHeader->addWidget(editTitle);
        efHeader->addStretch();

        QToolButton* btnSaveItem = new QToolButton(editFrame);
        btnSaveItem->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
        btnSaveItem->setToolTip(QStringLiteral("Item wijzigingen opslaan (Save)"));
        btnSaveItem->setAutoRaise(true);
        efHeader->addWidget(btnSaveItem);

        QToolButton* btnCancelItem = new QToolButton(editFrame);
        btnCancelItem->setIcon(IconUtils::getIcon(IconType::Cancel, QColor(220, 50, 50)));
        btnCancelItem->setToolTip(QStringLiteral("Annuleren (Cancel)"));
        btnCancelItem->setAutoRaise(true);
        efHeader->addWidget(btnCancelItem);

        efLayout->addLayout(efHeader);

        QFormLayout* form = new QFormLayout();
        form->setSpacing(6);

        QLineEdit* editTitleInput = new QLineEdit(editFrame);
        editTitleInput->setText(item->title);
        editTitleInput->setPlaceholderText(QStringLiteral("Titel van het item / website"));
        form->addRow(QStringLiteral("Titel:"), editTitleInput);

        QLineEdit* editUrlInput = new QLineEdit(editFrame);
        editUrlInput->setText(item->url);
        editUrlInput->setPlaceholderText(QStringLiteral("https://www.voorbeeld.nl"));
        form->addRow(QStringLiteral("Website / URL:"), editUrlInput);

        QLineEdit* editCatInput = new QLineEdit(editFrame);
        editCatInput->setText(item->category);
        editCatInput->setPlaceholderText(QStringLiteral("Bijv. Werk, Privé, Social, Bank"));
        form->addRow(QStringLiteral("Categorie:"), editCatInput);

        QString currentNotes;
        if (!item->notesEncrypted.isEmpty()) {
            core::VaultCrypto::decryptPassword(item->notesEncrypted, item->notesNonce, item->notesTag, m_masterKey, currentNotes);
        }
        QLineEdit* editNotesInput = new QLineEdit(editFrame);
        editNotesInput->setText(currentNotes);
        editNotesInput->setPlaceholderText(QStringLiteral("Optionele notities of opmerkingen"));
        form->addRow(QStringLiteral("Notities:"), editNotesInput);

        efLayout->addLayout(form);

        connect(btnSaveItem, &QToolButton::clicked, this, [this, item, editTitleInput, editUrlInput, editCatInput, editNotesInput]() {
            QString newTitle = editTitleInput->text().trimmed();
            if (newTitle.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Invoer vereist"), QStringLiteral("Voer een titel in voor dit item."));
                return;
            }
            item->title = newTitle;
            item->url = editUrlInput->text().trimmed();
            item->category = editCatInput->text().trimmed();

            QString notes = editNotesInput->text().trimmed();
            if (!notes.isEmpty()) {
                core::VaultCrypto::encryptPassword(notes, m_masterKey, item->notesEncrypted, item->notesNonce, item->notesTag);
            } else {
                item->notesEncrypted.clear();
                item->notesNonce.clear();
                item->notesTag.clear();
            }

            item->updatedAt = QDateTime::currentDateTimeUtc();
            m_modified = true;
            saveCurrentVault();
            m_editingItem = false;
            populateTree(m_searchEdit->text());
            populateAccountList(item);
        });

        connect(btnCancelItem, &QToolButton::clicked, this, [this, item]() {
            m_editingItem = false;
            populateAccountList(item);
        });

        m_itemHeaderLayout->addWidget(editFrame);
    }

    // --- ACCOUNTS SECTION ---
    // Check if adding new account (m_editingAccountIndex == -2)
    if (m_editingAccountIndex == -2) {
        QFrame* newCard = new QFrame(m_accountsContainer);
        newCard->setStyleSheet("QFrame { background-color: #f0fdf4; border: 1.5px solid #22c55e; border-radius: 6px; padding: 10px; }");
        QVBoxLayout* ncLayout = new QVBoxLayout(newCard);
        ncLayout->setSpacing(8);

        QHBoxLayout* ncHeader = new QHBoxLayout();
        QLabel* ncTitle = new QLabel(QStringLiteral("Nieuw Account Toevoegen"), newCard);
        ncTitle->setStyleSheet("font-weight: bold; font-size: 13px; color: #15803d;");
        ncHeader->addWidget(ncTitle);
        ncHeader->addStretch();

        QToolButton* btnSaveNew = new QToolButton(newCard);
        btnSaveNew->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
        btnSaveNew->setToolTip(QStringLiteral("Account opslaan (Save)"));
        btnSaveNew->setAutoRaise(true);
        ncHeader->addWidget(btnSaveNew);

        QToolButton* btnCancelNew = new QToolButton(newCard);
        btnCancelNew->setIcon(IconUtils::getIcon(IconType::Cancel, QColor(220, 50, 50)));
        btnCancelNew->setToolTip(QStringLiteral("Annuleren (Cancel)"));
        btnCancelNew->setAutoRaise(true);
        ncHeader->addWidget(btnCancelNew);

        ncLayout->addLayout(ncHeader);

        QFormLayout* nForm = new QFormLayout();
        nForm->setSpacing(6);

        QLineEdit* nLabelEdit = new QLineEdit(newCard);
        nLabelEdit->setPlaceholderText(QStringLiteral("Bijv. Privé, Werk, Hoofdaccount"));
        nForm->addRow(QStringLiteral("Label / Naam:"), nLabelEdit);

        QLineEdit* nUserEdit = new QLineEdit(newCard);
        nUserEdit->setPlaceholderText(QStringLiteral("Gebruikersnaam"));
        nForm->addRow(QStringLiteral("Gebruikersnaam:"), nUserEdit);

        QLineEdit* nEmailEdit = new QLineEdit(newCard);
        nEmailEdit->setPlaceholderText(QStringLiteral("naam@voorbeeld.nl"));
        nForm->addRow(QStringLiteral("E-mailadres:"), nEmailEdit);

        QCheckBox* nDefaultChk = new QCheckBox(QStringLiteral("Standaard e-mailadres (Default)"), newCard);
        nDefaultChk->setChecked(item->accounts.isEmpty());
        nForm->addRow(QStringLiteral(""), nDefaultChk);

        QHBoxLayout* nPassLayout = new QHBoxLayout();
        QLineEdit* nPassEdit = new QLineEdit(newCard);
        nPassEdit->setEchoMode(QLineEdit::Password);
        nPassEdit->setPlaceholderText(QStringLiteral("Wachtwoord"));
        nPassLayout->addWidget(nPassEdit, 1);

        QToolButton* nBtnReveal = new QToolButton(newCard);
        nBtnReveal->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
        nBtnReveal->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
        nBtnReveal->setAutoRaise(true);
        nPassLayout->addWidget(nBtnReveal);

        QToolButton* nBtnGen = new QToolButton(newCard);
        nBtnGen->setIcon(IconUtils::getIcon(IconType::Refresh, QColor(0, 122, 255)));
        nBtnGen->setToolTip(QStringLiteral("Genereer nieuw veilig wachtwoord"));
        nBtnGen->setAutoRaise(true);
        nPassLayout->addWidget(nBtnGen);

        nForm->addRow(QStringLiteral("Wachtwoord:"), nPassLayout);
        ncLayout->addLayout(nForm);

        connect(nBtnReveal, &QToolButton::pressed, this, [nPassEdit]() { nPassEdit->setEchoMode(QLineEdit::Normal); });
        connect(nBtnReveal, &QToolButton::released, this, [nPassEdit]() { nPassEdit->setEchoMode(QLineEdit::Password); });
        connect(nBtnGen, &QToolButton::clicked, this, [this, nPassEdit]() {
            std::string gen = core::PasswordGenerator::generate(m_vaultDoc.settings.toPasswordOptions());
            nPassEdit->setText(QString::fromStdString(gen));
        });

        connect(btnSaveNew, &QToolButton::clicked, this, [this, item, nLabelEdit, nUserEdit, nEmailEdit, nDefaultChk, nPassEdit]() {
            QString label = nLabelEdit->text().trimmed();
            QString user = nUserEdit->text().trimmed();
            QString email = nEmailEdit->text().trimmed();
            QString pass = nPassEdit->text();

            if (label.isEmpty() && user.isEmpty() && email.isEmpty() && pass.isEmpty()) {
                QMessageBox::warning(this, QStringLiteral("Invoer vereist"), QStringLiteral("Vul minimaal één veld in voor het nieuwe account."));
                return;
            }

            core::AccountEntry newAcc;
            newAcc.id = "acc_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
            newAcc.label = label;
            newAcc.username = user;
            newAcc.email = email;
            newAcc.isDefaultEmail = nDefaultChk->isChecked();
            newAcc.lastAccessed = QDateTime::currentDateTimeUtc();

            if (!pass.isEmpty()) {
                core::VaultCrypto::encryptPassword(pass, m_masterKey, newAcc.encryptedPassword, newAcc.nonce, newAcc.authTag);
            }

            if (newAcc.isDefaultEmail) {
                for (auto& acc : item->accounts) acc.isDefaultEmail = false;
            }

            item->accounts.append(newAcc);
            item->updatedAt = QDateTime::currentDateTimeUtc();
            item->lastAccessed = item->updatedAt;
            m_modified = true;
            saveCurrentVault();

            m_editingAccountIndex = -1;
            populateAccountList(item);
        });

        connect(btnCancelNew, &QToolButton::clicked, this, [this, item]() {
            m_editingAccountIndex = -1;
            populateAccountList(item);
        });

        m_accountsLayout->addWidget(newCard);
    }

    if (item->accounts.isEmpty() && m_editingAccountIndex != -2) {
        QLabel* emptyLabel = new QLabel(QStringLiteral("Nog geen accounts opgeslagen onder dit item. Klik op '+' om een account toe te voegen."), m_accountsContainer);
        emptyLabel->setStyleSheet("color: #888; font-style: italic; padding: 20px;");
        m_accountsLayout->addWidget(emptyLabel);
    } else {
        for (int i = 0; i < item->accounts.size(); ++i) {
            auto& acc = item->accounts[i];
            int accIdx = i;

            if (m_editingAccountIndex == accIdx) {
                // EDIT MODE for existing Account: Show ALL fields with Cancel and Save icon buttons
                QFrame* card = new QFrame(m_accountsContainer);
                card->setStyleSheet("QFrame { background-color: #f0f7ff; border: 1.5px solid #007aff; border-radius: 6px; padding: 10px; }");

                QVBoxLayout* cardLayout = new QVBoxLayout(card);
                cardLayout->setSpacing(8);

                QHBoxLayout* headerRow = new QHBoxLayout();
                QLabel* editHeader = new QLabel(QString("Account %1 Bewerken").arg(accIdx + 1), card);
                editHeader->setStyleSheet("font-weight: bold; font-size: 13px; color: #007aff;");
                headerRow->addWidget(editHeader);
                headerRow->addStretch();

                QToolButton* btnSaveAcc = new QToolButton(card);
                btnSaveAcc->setIcon(IconUtils::getIcon(IconType::Save, QColor(40, 167, 69)));
                btnSaveAcc->setToolTip(QStringLiteral("Account wijzigingen opslaan (Save)"));
                btnSaveAcc->setAutoRaise(true);
                headerRow->addWidget(btnSaveAcc);

                QToolButton* btnCancelAcc = new QToolButton(card);
                btnCancelAcc->setIcon(IconUtils::getIcon(IconType::Cancel, QColor(220, 50, 50)));
                btnCancelAcc->setToolTip(QStringLiteral("Annuleren (Cancel)"));
                btnCancelAcc->setAutoRaise(true);
                headerRow->addWidget(btnCancelAcc);

                cardLayout->addLayout(headerRow);

                QFormLayout* form = new QFormLayout();
                form->setSpacing(6);

                QLineEdit* editLabel = new QLineEdit(card);
                editLabel->setText(acc.label);
                editLabel->setPlaceholderText(QStringLiteral("Bijv. Privé, Werk, Hoofdaccount"));
                form->addRow(QStringLiteral("Label / Naam:"), editLabel);

                QLineEdit* editUser = new QLineEdit(card);
                editUser->setText(acc.username);
                editUser->setPlaceholderText(QStringLiteral("Gebruikersnaam"));
                form->addRow(QStringLiteral("Gebruikersnaam:"), editUser);

                QLineEdit* editEmail = new QLineEdit(card);
                editEmail->setText(acc.email);
                editEmail->setPlaceholderText(QStringLiteral("naam@voorbeeld.nl"));
                form->addRow(QStringLiteral("E-mailadres:"), editEmail);

                QCheckBox* chkDefault = new QCheckBox(QStringLiteral("Standaard e-mailadres (Default)"), card);
                chkDefault->setChecked(acc.isDefaultEmail);
                form->addRow(QStringLiteral(""), chkDefault);

                // Password row
                QString currentPass;
                core::VaultCrypto::decryptPassword(acc.encryptedPassword, acc.nonce, acc.authTag, m_masterKey, currentPass);

                QHBoxLayout* passLayout = new QHBoxLayout();
                QLineEdit* editPass = new QLineEdit(card);
                editPass->setEchoMode(QLineEdit::Password);
                editPass->setText(currentPass);
                editPass->setPlaceholderText(QStringLiteral("Wachtwoord"));
                passLayout->addWidget(editPass, 1);

                QToolButton* btnReveal = new QToolButton(card);
                btnReveal->setIcon(IconUtils::getIcon(IconType::Eye, QColor(70, 70, 70)));
                btnReveal->setToolTip(QStringLiteral("Wachtwoord tonen (ingedrukt houden)"));
                btnReveal->setAutoRaise(true);
                passLayout->addWidget(btnReveal);

                QToolButton* btnGen = new QToolButton(card);
                btnGen->setIcon(IconUtils::getIcon(IconType::Refresh, QColor(0, 122, 255)));
                btnGen->setToolTip(QStringLiteral("Genereer nieuw veilig wachtwoord"));
                btnGen->setAutoRaise(true);
                passLayout->addWidget(btnGen);

                form->addRow(QStringLiteral("Wachtwoord:"), passLayout);
                cardLayout->addLayout(form);

                connect(btnReveal, &QToolButton::pressed, this, [editPass]() { editPass->setEchoMode(QLineEdit::Normal); });
                connect(btnReveal, &QToolButton::released, this, [editPass]() { editPass->setEchoMode(QLineEdit::Password); });
                connect(btnGen, &QToolButton::clicked, this, [this, editPass]() {
                    std::string gen = core::PasswordGenerator::generate(m_vaultDoc.settings.toPasswordOptions());
                    editPass->setText(QString::fromStdString(gen));
                });

                connect(btnSaveAcc, &QToolButton::clicked, this, [this, item, accIdx, editLabel, editUser, editEmail, chkDefault, editPass]() {
                    QString label = editLabel->text().trimmed();
                    QString user = editUser->text().trimmed();
                    QString email = editEmail->text().trimmed();
                    QString pass = editPass->text();

                    if (label.isEmpty() && user.isEmpty() && email.isEmpty() && pass.isEmpty()) {
                        QMessageBox::warning(this, QStringLiteral("Invoer vereist"), QStringLiteral("Vul minimaal één veld in voor het account."));
                        return;
                    }

                    if (accIdx < item->accounts.size()) {
                        auto& a = item->accounts[accIdx];
                        a.label = label;
                        a.username = user;
                        a.email = email;
                        a.isDefaultEmail = chkDefault->isChecked();
                        a.lastAccessed = QDateTime::currentDateTimeUtc();

                        if (!pass.isEmpty()) {
                            core::VaultCrypto::encryptPassword(pass, m_masterKey, a.encryptedPassword, a.nonce, a.authTag);
                        } else {
                            a.encryptedPassword.clear();
                            a.nonce.clear();
                            a.authTag.clear();
                        }

                        if (a.isDefaultEmail) {
                            for (int k = 0; k < item->accounts.size(); ++k) {
                                if (k != accIdx) item->accounts[k].isDefaultEmail = false;
                            }
                        }

                        item->updatedAt = QDateTime::currentDateTimeUtc();
                        item->lastAccessed = item->updatedAt;
                        m_modified = true;
                        saveCurrentVault();
                    }

                    m_editingAccountIndex = -1;
                    populateAccountList(item);
                });

                connect(btnCancelAcc, &QToolButton::clicked, this, [this, item]() {
                    m_editingAccountIndex = -1;
                    populateAccountList(item);
                });

                m_accountsLayout->addWidget(card);
            } else {
                // VIEW MODE: Show ONLY filled fields!
                QFrame* card = new QFrame(m_accountsContainer);
                card->setFrameShape(QFrame::StyledPanel);
                card->setStyleSheet("QFrame { background-color: #f8f9fa; border: 1px solid #dee2e6; border-radius: 6px; padding: 6px; }");

                QVBoxLayout* cardLayout = new QVBoxLayout(card);
                cardLayout->setSpacing(6);

                // Row 1: Label + Default badge + Edit icon button + Delete icon button
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

                // Edit Account icon button
                QToolButton* btnEditAcc = new QToolButton(card);
                btnEditAcc->setIcon(IconUtils::getIcon(IconType::Edit, QColor(70, 70, 70)));
                btnEditAcc->setToolTip(QStringLiteral("Account bewerken"));
                btnEditAcc->setAutoRaise(true);
                headerRow->addWidget(btnEditAcc);

                // Delete Account icon button
                QToolButton* btnDelAcc = new QToolButton(card);
                btnDelAcc->setIcon(IconUtils::getIcon(IconType::Trash, QColor(220, 50, 50)));
                btnDelAcc->setToolTip(QStringLiteral("Account verwijderen"));
                btnDelAcc->setAutoRaise(true);
                headerRow->addWidget(btnDelAcc);

                cardLayout->addLayout(headerRow);

                // Row 2: Username (only if filled)
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

                // Row 3: Email (only if filled)
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

                // Row 4: Password row
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

                // Row 5: Last accessed
                QLabel* accessLabel = new QLabel(QString("Laatst geopend: %1").arg(formatTimestamp(acc.lastAccessed)), card);
                accessLabel->setStyleSheet("color: #777; font-size: 11px;");
                cardLayout->addWidget(accessLabel);

                auto decryptHelper = [this, acc, item, accIdx]() -> QString {
                    QString plaintext;
                    if (!core::VaultCrypto::decryptPassword(acc.encryptedPassword, acc.nonce, acc.authTag, m_masterKey, plaintext)) {
                        return QString();
                    }
                    QDateTime now = QDateTime::currentDateTimeUtc();
                    if (item && accIdx < item->accounts.size()) {
                        item->accounts[accIdx].lastAccessed = now;
                        item->lastAccessed = now;
                        m_modified = true;
                        saveCurrentVault();
                    }
                    return plaintext;
                };

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

                connect(btnEditAcc, &QToolButton::clicked, this, [this, accIdx, item]() {
                    m_editingAccountIndex = accIdx;
                    populateAccountList(item);
                });
                connect(btnDelAcc, &QToolButton::clicked, this, [this, accIdx]() {
                    onDeleteAccount(accIdx);
                });

                m_accountsLayout->addWidget(card);
            }
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
        m_editingItem = true;
        populateAccountList(item);
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

    m_editingAccountIndex = -2; // Start inline addition
    populateAccountList(item);
}

void VaultDialog::onEditAccount(int accountIndex) {
    core::VaultItem* item = getSelectedItem();
    if (!item || accountIndex < 0 || accountIndex >= item->accounts.size()) return;

    m_editingAccountIndex = accountIndex; // Start inline edit
    populateAccountList(item);
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
