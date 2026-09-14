#include "core/VaultModel.h"
#include "core/SecureRandom.h"
#include <QUuid>

namespace core {

namespace {
    QString generateId(const QString& prefix) {
        return prefix + "_" + QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
}

// ============================================================================
// AccountEntry
// ============================================================================
QJsonObject AccountEntry::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["label"] = label;
    obj["username"] = username;
    obj["email"] = email;
    obj["is_default_email"] = isDefaultEmail;
    obj["encrypted_password"] = encryptedPassword;
    obj["nonce"] = nonce;
    obj["auth_tag"] = authTag;
    if (lastAccessed.isValid()) {
        obj["last_accessed"] = lastAccessed.toString(Qt::ISODateWithMs);
    }
    return obj;
}

AccountEntry AccountEntry::fromJson(const QJsonObject& obj) {
    AccountEntry acc;
    acc.id = obj.value("id").toString();
    if (acc.id.isEmpty()) {
        acc.id = generateId("acc");
    }
    acc.label = obj.value("label").toString();
    acc.username = obj.value("username").toString();
    acc.email = obj.value("email").toString();
    acc.isDefaultEmail = obj.value("is_default_email").toBool(false);
    acc.encryptedPassword = obj.value("encrypted_password").toString();
    acc.nonce = obj.value("nonce").toString();
    acc.authTag = obj.value("auth_tag").toString();
    if (obj.contains("last_accessed")) {
        acc.lastAccessed = QDateTime::fromString(obj.value("last_accessed").toString(), Qt::ISODateWithMs);
    }
    return acc;
}

// ============================================================================
// VaultItem
// ============================================================================
QJsonObject VaultItem::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["title"] = title;
    obj["url"] = url;
    obj["category"] = category;
    if (createdAt.isValid()) {
        obj["created_at"] = createdAt.toString(Qt::ISODateWithMs);
    }
    if (updatedAt.isValid()) {
        obj["updated_at"] = updatedAt.toString(Qt::ISODateWithMs);
    }
    if (lastAccessed.isValid()) {
        obj["last_accessed"] = lastAccessed.toString(Qt::ISODateWithMs);
    }
    obj["notes_encrypted"] = notesEncrypted;
    obj["notes_nonce"] = notesNonce;
    obj["notes_tag"] = notesTag;

    QJsonArray accArray;
    for (const auto& acc : accounts) {
        accArray.append(acc.toJson());
    }
    obj["accounts"] = accArray;
    return obj;
}

VaultItem VaultItem::fromJson(const QJsonObject& obj) {
    VaultItem item;
    item.id = obj.value("id").toString();
    if (item.id.isEmpty()) {
        item.id = generateId("item");
    }
    item.title = obj.value("title").toString();
    item.url = obj.value("url").toString();
    item.category = obj.value("category").toString();
    if (obj.contains("created_at")) {
        item.createdAt = QDateTime::fromString(obj.value("created_at").toString(), Qt::ISODateWithMs);
    }
    if (obj.contains("updated_at")) {
        item.updatedAt = QDateTime::fromString(obj.value("updated_at").toString(), Qt::ISODateWithMs);
    }
    if (obj.contains("last_accessed")) {
        item.lastAccessed = QDateTime::fromString(obj.value("last_accessed").toString(), Qt::ISODateWithMs);
    }
    item.notesEncrypted = obj.value("notes_encrypted").toString();
    item.notesNonce = obj.value("notes_nonce").toString();
    item.notesTag = obj.value("notes_tag").toString();

    QJsonArray accArray = obj.value("accounts").toArray();
    for (const auto& val : accArray) {
        if (val.isObject()) {
            item.accounts.append(AccountEntry::fromJson(val.toObject()));
        }
    }
    return item;
}

AccountEntry* VaultItem::findDefaultAccount() {
    for (auto& acc : accounts) {
        if (acc.isDefaultEmail) {
            return &acc;
        }
    }
    return accounts.isEmpty() ? nullptr : &accounts.first();
}

const AccountEntry* VaultItem::findDefaultAccount() const {
    for (const auto& acc : accounts) {
        if (acc.isDefaultEmail) {
            return &acc;
        }
    }
    return accounts.isEmpty() ? nullptr : &accounts.first();
}

// ============================================================================
// VaultGroup
// ============================================================================
QJsonObject VaultGroup::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["name"] = name;
    QJsonArray itemArray;
    for (const auto& item : items) {
        itemArray.append(item.toJson());
    }
    obj["items"] = itemArray;
    return obj;
}

VaultGroup VaultGroup::fromJson(const QJsonObject& obj) {
    VaultGroup group;
    group.id = obj.value("id").toString();
    if (group.id.isEmpty()) {
        group.id = generateId("grp");
    }
    group.name = obj.value("name").toString();
    QJsonArray itemArray = obj.value("items").toArray();
    for (const auto& val : itemArray) {
        if (val.isObject()) {
            group.items.append(VaultItem::fromJson(val.toObject()));
        }
    }
    return group;
}

// ============================================================================
// VaultSettings
// ============================================================================
PasswordOptions VaultSettings::toPasswordOptions() const {
    PasswordOptions opt;
    opt.length = lastGeneratorLength;
    opt.useLetters = useLetters;
    opt.letterCase = (letterCase == 1) ? LetterCase::LowerOnly :
                     (letterCase == 2) ? LetterCase::UpperOnly :
                                         LetterCase::Both;
    opt.useDigits = useDigits;
    opt.useSpecialChars = useSpecialChars;
    opt.specialCharSet = specialCharSet.toStdString();
    opt.hexOnly = hexOnly;
    opt.hexCase = (hexCase == 1) ? HexCase::Lowercase : HexCase::Uppercase;
    opt.useSignature = useSignature;
    opt.signaturePosition = signaturePosition;
    opt.sanitize();
    return opt;
}

void VaultSettings::fromPasswordOptions(const PasswordOptions& options) {
    lastGeneratorLength = options.length;
    useLetters = options.useLetters;
    letterCase = (options.letterCase == LetterCase::LowerOnly) ? 1 :
                 (options.letterCase == LetterCase::UpperOnly) ? 2 : 0;
    useDigits = options.useDigits;
    useSpecialChars = options.useSpecialChars;
    specialCharSet = QString::fromStdString(options.specialCharSet);
    hexOnly = options.hexOnly;
    hexCase = (options.hexCase == HexCase::Lowercase) ? 1 : 0;
    useSignature = options.useSignature;
    signaturePosition = options.signaturePosition;
}

QJsonObject VaultSettings::toJson() const {
    QJsonObject obj;
    obj["last_generator_length"] = lastGeneratorLength;
    obj["use_letters"] = useLetters;
    obj["letter_case"] = letterCase;
    obj["use_digits"] = useDigits;
    obj["use_special_chars"] = useSpecialChars;
    obj["special_char_set"] = specialCharSet;
    obj["hex_only"] = hexOnly;
    obj["hex_case"] = hexCase;
    obj["use_signature"] = useSignature;
    obj["signature_position"] = signaturePosition;
    obj["require_biometrics_for_reveal"] = requireBiometricsForReveal;
    obj["require_biometrics_for_copy"] = requireBiometricsForCopy;
    obj["require_biometrics_for_vault"] = requireBiometricsForVault;
    obj["preferred_provider"] = static_cast<int>(preferredProvider);
    obj["custom_vault_path"] = customVaultPath;
    return obj;
}

VaultSettings VaultSettings::fromJson(const QJsonObject& obj) {
    VaultSettings s;
    s.lastGeneratorLength = obj.value("last_generator_length").toInt(16);
    s.useLetters = obj.value("use_letters").toBool(true);
    s.letterCase = obj.value("letter_case").toInt(0);
    s.useDigits = obj.value("use_digits").toBool(true);
    s.useSpecialChars = obj.value("use_special_chars").toBool(true);
    s.specialCharSet = obj.value("special_char_set").toString(QStringLiteral("!@#$%^&*()_+-=[]{}|;:,.<>?/~"));
    s.hexOnly = obj.value("hex_only").toBool(false);
    s.hexCase = obj.value("hex_case").toInt(0);
    s.useSignature = obj.value("use_signature").toBool(false);
    s.signaturePosition = obj.value("signature_position").toInt(2);
    s.requireBiometricsForReveal = obj.value("require_biometrics_for_reveal").toBool(false);
    s.requireBiometricsForCopy = obj.value("require_biometrics_for_copy").toBool(false);
    s.requireBiometricsForVault = obj.value("require_biometrics_for_vault").toBool(false);
    s.preferredProvider = static_cast<CloudProvider>(obj.value("preferred_provider").toInt(static_cast<int>(CloudProvider::OneDrive)));
    s.customVaultPath = obj.value("custom_vault_path").toString();
    return s;
}

// ============================================================================
// VaultDocument
// ============================================================================
QJsonObject VaultDocument::toJson() const {
    QJsonObject obj;
    obj["version"] = version;
    obj["kdf_salt"] = kdfSalt;
    obj["kdf_iterations"] = kdfIterations;
    if (lastSynced.isValid()) {
        obj["last_synced"] = lastSynced.toString(Qt::ISODateWithMs);
    }
    obj["settings"] = settings.toJson();

    QJsonArray grpArray;
    for (const auto& grp : groups) {
        grpArray.append(grp.toJson());
    }
    obj["groups"] = grpArray;
    return obj;
}

VaultDocument VaultDocument::fromJson(const QJsonObject& obj) {
    VaultDocument doc;
    doc.version = obj.value("version").toInt(1);
    doc.kdfSalt = obj.value("kdf_salt").toString();
    doc.kdfIterations = obj.value("kdf_iterations").toInt(100000);
    if (obj.contains("last_synced")) {
        doc.lastSynced = QDateTime::fromString(obj.value("last_synced").toString(), Qt::ISODateWithMs);
    }
    if (obj.contains("settings") && obj.value("settings").isObject()) {
        doc.settings = VaultSettings::fromJson(obj.value("settings").toObject());
    }
    QJsonArray grpArray = obj.value("groups").toArray();
    for (const auto& val : grpArray) {
        if (val.isObject()) {
            doc.groups.append(VaultGroup::fromJson(val.toObject()));
        }
    }
    return doc;
}

QByteArray VaultDocument::toJsonData(QJsonDocument::JsonFormat format) const {
    QJsonDocument doc(toJson());
    return doc.toJson(format);
}

VaultDocument VaultDocument::fromJsonData(const QByteArray& data, bool* ok) {
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(data, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        if (ok) *ok = false;
        return VaultDocument::createDefault();
    }
    if (ok) *ok = true;
    return VaultDocument::fromJson(doc.object());
}

VaultDocument VaultDocument::createDefault() {
    VaultDocument doc;
    doc.version = 1;
    doc.lastSynced = QDateTime::currentDateTimeUtc();
    doc.kdfIterations = 100000;

    // Generate random 16-byte KDF salt
    uint8_t saltBytes[16];
    SecureRandom::getBytes(saltBytes, sizeof(saltBytes));
    doc.kdfSalt = QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(saltBytes), sizeof(saltBytes)).toBase64());

    // Default Groups
    VaultGroup general;
    general.id = generateId("grp");
    general.name = QStringLiteral("Algemeen / General");
    doc.groups.append(general);

    VaultGroup web;
    web.id = generateId("grp");
    web.name = QStringLiteral("Web & Social Media");
    doc.groups.append(web);

    VaultGroup work;
    work.id = generateId("grp");
    work.name = QStringLiteral("Werk / Work");
    doc.groups.append(work);

    return doc;
}

} // namespace core
