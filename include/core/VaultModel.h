#pragma once

#include <QString>
#include <QList>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "core/PasswordOptions.h"

namespace core {

struct AccountEntry {
    QString id;
    QString label;
    QString username;
    QString email;
    bool isDefaultEmail = false;
    QString encryptedPassword; // Base64
    QString nonce;             // Base64
    QString authTag;           // Base64
    QDateTime lastAccessed;

    QJsonObject toJson() const;
    static AccountEntry fromJson(const QJsonObject& obj);
};

struct VaultItem {
    QString id;
    QString title;
    QString url;
    QString category;
    QDateTime createdAt;
    QDateTime updatedAt;
    QDateTime lastAccessed;
    QList<AccountEntry> accounts;
    QString notesEncrypted;
    QString notesNonce;
    QString notesTag;

    QJsonObject toJson() const;
    static VaultItem fromJson(const QJsonObject& obj);

    // Helpers
    AccountEntry* findDefaultAccount();
    const AccountEntry* findDefaultAccount() const;
};

struct VaultGroup {
    QString id;
    QString name;
    QList<VaultItem> items;

    QJsonObject toJson() const;
    static VaultGroup fromJson(const QJsonObject& obj);
};

enum class CloudProvider {
    Local = 0,
    OneDrive = 1,
    GoogleDrive = 2,
    Custom = 3
};

struct VaultSettings {
    int lastGeneratorLength = 16;
    bool useLetters = true;
    int letterCase = 0; // 0 = Both, 1 = LowerOnly, 2 = UpperOnly
    bool useDigits = true;
    bool useSpecialChars = true;
    QString specialCharSet = QStringLiteral("!@#$%^&*()_+-=[]{}|;:,.<>?/~");
    bool hexOnly = false;
    int hexCase = 0; // 0 = Uppercase, 1 = Lowercase
    bool useSignature = false;
    int signaturePosition = 2;

    // Security & Biometrics
    bool requireBiometricsForReveal = false;
    bool requireBiometricsForCopy = false;
    bool requireBiometricsForVault = false;

    // Cloud storage preferences
    CloudProvider preferredProvider = CloudProvider::OneDrive;
    QString customVaultPath;

    PasswordOptions toPasswordOptions() const;
    void fromPasswordOptions(const PasswordOptions& options);

    QJsonObject toJson() const;
    static VaultSettings fromJson(const QJsonObject& obj);
};

struct VaultDocument {
    int version = 1;
    QString kdfSalt; // Base64
    int kdfIterations = 100000;
    QDateTime lastSynced;
    VaultSettings settings;
    QList<VaultGroup> groups;

    QJsonObject toJson() const;
    static VaultDocument fromJson(const QJsonObject& obj);

    QByteArray toJsonData(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const;
    static VaultDocument fromJsonData(const QByteArray& data, bool* ok = nullptr);

    // Creates an empty default vault structure
    static VaultDocument createDefault();
};

} // namespace core
