#include "core/VaultStorage.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QLockFile>
#include <QProcessEnvironment>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

namespace core {

namespace {
    const int MAX_BACKUPS = 10;
}

QString VaultStorage::getLocalConfigDirectory() {
    QString configDir = QDir::homePath() + "/.wols_password_generator";
    QDir().mkpath(configDir);
    return configDir;
}

QString VaultStorage::getLocalConfigPath() {
    return getLocalConfigDirectory() + "/config.json";
}

QStringList VaultStorage::getAvailableOneDriveDirectories() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList candidates;

    // Check environment variables on Windows
    if (env.contains("OneDriveCommercial")) candidates << env.value("OneDriveCommercial");
    if (env.contains("OneDriveConsumer")) candidates << env.value("OneDriveConsumer");
    if (env.contains("OneDrive")) candidates << env.value("OneDrive");

    // Scan user profile and home directory for all subdirectories matching OneDrive*
    QStringList baseDirs;
    QString userProfile = env.value("USERPROFILE");
    if (!userProfile.isEmpty()) baseDirs << userProfile;
    baseDirs << QDir::homePath();

    for (const QString& base : baseDirs) {
        if (base.isEmpty() || !QDir(base).exists()) continue;
        QDir bDir(base);
        QStringList onedriveDirs = bDir.entryList(QStringList() << "OneDrive*", QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString& d : onedriveDirs) {
            candidates << base + "/" + d;
        }
    }

    QStringList result;
    for (const QString& path : candidates) {
        QString clean = QDir::cleanPath(path);
        if (!clean.isEmpty() && QDir(clean).exists()) {
            if (!result.contains(clean)) {
                result.append(clean);
            }
        }
    }
    return result;
}

QString VaultStorage::getOneDriveDirectory(const QString& preferredDir) {
    if (!preferredDir.trimmed().isEmpty()) {
        QString clean = QDir::cleanPath(preferredDir.trimmed());
        if (QDir(clean).exists()) {
            return clean;
        }
    }

    QStringList available = getAvailableOneDriveDirectories();
    if (!available.isEmpty()) {
        return available.first();
    }
    return QString();
}

QString VaultStorage::getOneDriveVaultPath(const QString& preferredDir) {
    QString oneDriveDir = getOneDriveDirectory(preferredDir);
    if (oneDriveDir.isEmpty()) {
        return QString();
    }
    QString folder = oneDriveDir + "/WolsPasswordManager";
    QDir().mkpath(folder);
    return folder + "/vault.json";
}

QStringList VaultStorage::getAvailableGoogleDriveDirectories() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList candidates;

    // Check Windows mounted drive letters commonly used by Google Drive (D: through Z:)
    for (char c = 'D'; c <= 'Z'; ++c) {
        QString drive = QString(c) + ":";
        candidates << drive + "/My Drive"
                   << drive + "/Mijn Drive"
                   << drive + "/Google Drive"
                   << drive;
    }

    // Check user profile paths
    QString userProfile = env.value("USERPROFILE");
    if (!userProfile.isEmpty()) {
        candidates << userProfile + "/Google Drive"
                   << userProfile + "/Google Drive/My Drive"
                   << userProfile + "/Google Drive/Mijn Drive"
                   << userProfile + "/My Drive"
                   << userProfile + "/Mijn Drive";

        // Check CloudStorage directory
        QDir cloudStorageDir(userProfile + "/CloudStorage");
        if (cloudStorageDir.exists()) {
            QStringList gdriveDirs = cloudStorageDir.entryList(QStringList() << "GoogleDrive*", QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QString& d : gdriveDirs) {
                candidates << userProfile + "/CloudStorage/" + d + "/My Drive";
                candidates << userProfile + "/CloudStorage/" + d + "/Mijn Drive";
                candidates << userProfile + "/CloudStorage/" + d;
            }
        }
    }

    // Check home directory for Linux/macOS
    candidates << QDir::homePath() + "/Google Drive"
               << QDir::homePath() + "/Google Drive/My Drive"
               << QDir::homePath() + "/Google Drive/Mijn Drive"
               << QDir::homePath() + "/GoogleDrive"
               << QDir::homePath() + "/My Drive"
               << QDir::homePath() + "/Mijn Drive";

    QStringList result;
    for (const QString& path : candidates) {
        QString clean = QDir::cleanPath(path);
        if (!clean.isEmpty() && QDir(clean).exists()) {
            if (!result.contains(clean)) {
                result.append(clean);
            }
        }
    }
    return result;
}

QString VaultStorage::getGoogleDriveDirectory() {
    QStringList available = getAvailableGoogleDriveDirectories();
    if (!available.isEmpty()) {
        return available.first();
    }
    return QString();
}

QString VaultStorage::findExistingVault(CloudProvider* outProvider, QString* outOneDriveDir) {
    auto isPopulatedVault = [](const QString& filePath) -> bool {
        if (!QFile::exists(filePath)) return false;
        QFileInfo fi(filePath);
        if (fi.size() == 0) return false;
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly)) {
            QByteArray data = file.read(1024);
            file.close();
            if (data.contains("kdf_salt") || data.contains("groups") || data.contains("version")) {
                return true;
            }
        }
        return fi.size() > 10;
    };

    // 1. Prioritize existing Google Drive vault locations
    QStringList gdriveDirs = getAvailableGoogleDriveDirectories();
    for (const QString& gdir : gdriveDirs) {
        QStringList gpaths;
        gpaths << gdir + "/WolsPasswordManager/vault.json"
               << gdir + "/vault.json";
        for (const QString& p : gpaths) {
            QString clean = QDir::cleanPath(p);
            if (isPopulatedVault(clean)) {
                if (outProvider) *outProvider = CloudProvider::GoogleDrive;
                return clean;
            }
        }
    }

    // 2. Check OneDrive locations
    QStringList onedriveDirs = getAvailableOneDriveDirectories();
    for (const QString& odir : onedriveDirs) {
        QStringList odpaths;
        odpaths << odir + "/WolsPasswordManager/vault.json"
                << odir + "/vault.json"
                << odir + "/Documents/WolsPasswordManager/vault.json"
                << odir + "/Documenten/WolsPasswordManager/vault.json";
        for (const QString& p : odpaths) {
            QString clean = QDir::cleanPath(p);
            if (isPopulatedVault(clean)) {
                if (outProvider) *outProvider = CloudProvider::OneDrive;
                if (outOneDriveDir) *outOneDriveDir = odir;
                return clean;
            }
        }
    }

    // 3. Check Local Documents
    QString localPath = getLocalVaultPath();
    if (isPopulatedVault(localPath)) {
        if (outProvider) *outProvider = CloudProvider::Local;
        return localPath;
    }

    QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docsDir.isEmpty()) {
        QString rootDocVault = QDir::cleanPath(docsDir + "/vault.json");
        if (isPopulatedVault(rootDocVault)) {
            if (outProvider) *outProvider = CloudProvider::Local;
            return rootDocVault;
        }
    }

    return QString();
}

QString VaultStorage::getGoogleDriveVaultPath() {
    QString gdriveDir = getGoogleDriveDirectory();
    if (gdriveDir.isEmpty()) {
        return QString();
    }
    QString folder = gdriveDir + "/WolsPasswordManager";
    QDir().mkpath(folder);
    return folder + "/vault.json";
}

QString VaultStorage::getLocalVaultPath() {
    QString docsDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (docsDir.isEmpty()) {
        docsDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    }
    if (docsDir.isEmpty()) {
        docsDir = QDir::homePath() + "/Documents";
    }
    QString vaultFolder = docsDir + "/WolsPasswordManager";
    QDir().mkpath(vaultFolder);
    return vaultFolder + "/vault.json";
}

bool VaultStorage::isCloudPathAvailable(CloudProvider provider, const QString& preferredOneDriveDir) {
    switch (provider) {
        case CloudProvider::OneDrive:
            return !getOneDriveDirectory(preferredOneDriveDir).isEmpty();
        case CloudProvider::GoogleDrive:
            return !getGoogleDriveDirectory().isEmpty();
        case CloudProvider::Local:
            return true;
        case CloudProvider::Custom:
            return true;
    }
    return false;
}

QString VaultStorage::resolveVaultPath(CloudProvider provider, const QString& customPath, const QString& preferredOneDriveDir) {
    switch (provider) {
        case CloudProvider::OneDrive: {
            QString path = getOneDriveVaultPath(preferredOneDriveDir);
            if (!path.isEmpty()) return path;
            break;
        }
        case CloudProvider::GoogleDrive: {
            QString path = getGoogleDriveVaultPath();
            if (!path.isEmpty()) return path;
            break;
        }
        case CloudProvider::Custom: {
            if (!customPath.trimmed().isEmpty()) {
                return customPath.trimmed();
            }
            break;
        }
        case CloudProvider::Local:
        default:
            return getLocalVaultPath();
    }
    return getDefaultVaultPath(preferredOneDriveDir);
}

QString VaultStorage::getDefaultVaultPath(const QString& preferredOneDriveDir) {
    // 1. Prioritize OneDrive if available
    QString oneDrivePath = getOneDriveVaultPath(preferredOneDriveDir);
    if (!oneDrivePath.isEmpty()) {
        return oneDrivePath;
    }

    // 2. Fallback to Google Drive if available
    QString gDrivePath = getGoogleDriveVaultPath();
    if (!gDrivePath.isEmpty()) {
        return gDrivePath;
    }

    // 3. Fallback to Local Documents
    return getLocalVaultPath();
}

bool VaultStorage::saveLocalConfig(const QString& activeVaultPath, CloudProvider provider, const QString& selectedOneDriveDir, const VaultSettings& settings, QString* outError) {
    QString configPath = getLocalConfigPath();
    QFileInfo fi(configPath);
    QDir().mkpath(fi.absolutePath());

    QJsonObject root;
    root["version"] = 1;
    root["active_vault_path"] = activeVaultPath;
    root["preferred_provider"] = static_cast<int>(provider);
    root["selected_onedrive_dir"] = selectedOneDriveDir;
    root["settings"] = settings.toJson();

    // Cross-platform configurations for Windows, Linux, and Android
    QJsonObject platformsObj;

    QJsonObject winObj;
    winObj["os"] = "Windows";
    winObj["config_path"] = "%USERPROFILE%/.wols_password_generator/config.json";
    winObj["default_vault_path"] = "%USERPROFILE%/OneDrive/WolsPasswordManager/vault.json";
    winObj["local_vault_path"] = "%USERPROFILE%/Documents/WolsPasswordManager/vault.json";
    winObj["cloud_sync_folder"] = selectedOneDriveDir.isEmpty() ? "%USERPROFILE%/OneDrive" : selectedOneDriveDir;
    winObj["preferred_provider"] = static_cast<int>(provider);
    platformsObj["windows"] = winObj;

    QJsonObject linuxObj;
    linuxObj["os"] = "Linux";
    linuxObj["config_path"] = "~/.config/wols_password_generator/config.json";
    linuxObj["default_vault_path"] = "~/OneDrive/WolsPasswordManager/vault.json";
    linuxObj["local_vault_path"] = "~/Documents/WolsPasswordManager/vault.json";
    linuxObj["cloud_sync_folder"] = "~/OneDrive";
    linuxObj["preferred_provider"] = static_cast<int>(provider);
    platformsObj["linux"] = linuxObj;

    QJsonObject androidObj;
    androidObj["os"] = "Android";
    androidObj["config_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/config.json";
    androidObj["default_vault_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/vault.json";
    androidObj["local_vault_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/vault.json";
    androidObj["cloud_sync_folder"] = "OneDrive / Google Drive Cloud App Sync";
    androidObj["preferred_provider"] = static_cast<int>(provider);
    platformsObj["android"] = androidObj;

    root["platforms"] = platformsObj;

    QJsonDocument doc(root);
    QByteArray jsonData = doc.toJson(QJsonDocument::Indented);

    QString tempPath = configPath + ".tmp";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) *outError = QStringLiteral("Kan configuratiebestand niet schrijven: ") + tempFile.errorString();
        return false;
    }
    if (tempFile.write(jsonData) != jsonData.size()) {
        if (outError) *outError = QStringLiteral("Fout bij schrijven van configuratiedata.");
        tempFile.close();
        QFile::remove(tempPath);
        return false;
    }
    tempFile.flush();
    tempFile.close();

    if (QFile::exists(configPath)) {
        QFile::remove(configPath);
    }
    if (!tempFile.rename(configPath)) {
        if (!QFile::copy(tempPath, configPath)) {
            if (outError) *outError = QStringLiteral("Kan configuratiebestand niet hernoemen/vervangen.");
            QFile::remove(tempPath);
            return false;
        }
        QFile::remove(tempPath);
    }

    return true;
}

bool VaultStorage::loadLocalConfig(QString& outVaultPath, CloudProvider& outProvider, QString& outSelectedOneDriveDir, VaultSettings& outSettings, QString* outError) {
    QString configPath = getLocalConfigPath();
    if (!QFile::exists(configPath)) {
        if (outError) *outError = QStringLiteral("Geen lokaal configuratiebestand gevonden.");
        return false;
    }

    QFile file(configPath);
    if (!file.open(QIODevice::ReadOnly)) {
        if (outError) *outError = QStringLiteral("Kan lokaal configuratiebestand niet openen: ") + file.errorString();
        return false;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonParseError parseErr;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseErr);
    if (parseErr.error != QJsonParseError::NoError || !doc.isObject()) {
        if (outError) *outError = QStringLiteral("Ongeldig JSON-formaat in configuratiebestand.");
        return false;
    }

    QJsonObject root = doc.object();
    outVaultPath = root.value("active_vault_path").toString();
    outProvider = static_cast<CloudProvider>(root.value("preferred_provider").toInt(static_cast<int>(CloudProvider::OneDrive)));
    outSelectedOneDriveDir = root.value("selected_onedrive_dir").toString();

    if (root.contains("settings") && root.value("settings").isObject()) {
        outSettings = VaultSettings::fromJson(root.value("settings").toObject());
    }

    return true;
}

QString VaultStorage::generatePlatformConfigJsonString(const QString& targetPlatform, const QString& activeVaultPath, CloudProvider provider, const QString& selectedOneDriveDir, const VaultSettings& settings) {
    QJsonObject root;
    root["version"] = 1;
    root["active_vault_path"] = activeVaultPath;
    root["preferred_provider"] = static_cast<int>(provider);
    root["selected_onedrive_dir"] = selectedOneDriveDir;
    root["settings"] = settings.toJson();

    QString plat = targetPlatform.toLower().trimmed();
    if (plat == "windows") {
        QJsonObject winObj;
        winObj["os"] = "Windows";
        winObj["config_path"] = "%USERPROFILE%/.wols_password_generator/config.json";
        winObj["default_vault_path"] = "%USERPROFILE%/OneDrive/WolsPasswordManager/vault.json";
        winObj["local_vault_path"] = "%USERPROFILE%/Documents/WolsPasswordManager/vault.json";
        winObj["cloud_sync_folder"] = selectedOneDriveDir.isEmpty() ? "%USERPROFILE%/OneDrive" : selectedOneDriveDir;
        winObj["preferred_provider"] = static_cast<int>(provider);
        root["platform"] = winObj;
    } else if (plat == "linux") {
        QJsonObject linuxObj;
        linuxObj["os"] = "Linux";
        linuxObj["config_path"] = "~/.config/wols_password_generator/config.json";
        linuxObj["default_vault_path"] = "~/OneDrive/WolsPasswordManager/vault.json";
        linuxObj["local_vault_path"] = "~/Documents/WolsPasswordManager/vault.json";
        linuxObj["cloud_sync_folder"] = "~/OneDrive";
        linuxObj["preferred_provider"] = static_cast<int>(provider);
        root["platform"] = linuxObj;
    } else if (plat == "android") {
        QJsonObject androidObj;
        androidObj["os"] = "Android";
        androidObj["config_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/config.json";
        androidObj["default_vault_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/vault.json";
        androidObj["local_vault_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/vault.json";
        androidObj["cloud_sync_folder"] = "OneDrive / Google Drive Cloud App Sync";
        androidObj["preferred_provider"] = static_cast<int>(provider);
        root["platform"] = androidObj;
    } else {
        // All platforms
        QJsonObject platformsObj;

        QJsonObject winObj;
        winObj["os"] = "Windows";
        winObj["config_path"] = "%USERPROFILE%/.wols_password_generator/config.json";
        winObj["default_vault_path"] = "%USERPROFILE%/OneDrive/WolsPasswordManager/vault.json";
        winObj["cloud_sync_folder"] = selectedOneDriveDir.isEmpty() ? "%USERPROFILE%/OneDrive" : selectedOneDriveDir;
        platformsObj["windows"] = winObj;

        QJsonObject linuxObj;
        linuxObj["os"] = "Linux";
        linuxObj["config_path"] = "~/.config/wols_password_generator/config.json";
        linuxObj["default_vault_path"] = "~/OneDrive/WolsPasswordManager/vault.json";
        linuxObj["cloud_sync_folder"] = "~/OneDrive";
        platformsObj["linux"] = linuxObj;

        QJsonObject androidObj;
        androidObj["os"] = "Android";
        androidObj["config_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/config.json";
        androidObj["default_vault_path"] = "/storage/emulated/0/Documents/WolsPasswordManager/vault.json";
        androidObj["cloud_sync_folder"] = "OneDrive / Google Drive Cloud App Sync";
        platformsObj["android"] = androidObj;

        root["platforms"] = platformsObj;
    }

    QJsonDocument doc(root);
    return QString::fromUtf8(doc.toJson(QJsonDocument::Indented));
}

QString VaultStorage::getBackupDirectory() {
    QString appData = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (appData.isEmpty()) {
        appData = QDir::homePath() + "/.wols_password_manager";
    }
    QString backupDir = appData + "/backups";
    QDir().mkpath(backupDir);
    return backupDir;
}

VaultDocument VaultStorage::loadVault(const QString& filePath, QString* outError) {
    QFile file(filePath);
    if (!file.exists()) {
        // Return a fresh default vault
        return VaultDocument::createDefault();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        if (outError) {
            *outError = QStringLiteral("Kan kluisbestand niet openen om te lezen: ") + file.errorString();
        }
        return VaultDocument::createDefault();
    }

    QByteArray data = file.readAll();
    file.close();

    bool ok = false;
    VaultDocument doc = VaultDocument::fromJsonData(data, &ok);
    if (!ok) {
        if (outError) {
            *outError = QStringLiteral("Ongeldig JSON-formaat in kluisbestand.");
        }
    }
    return doc;
}

bool VaultStorage::saveVault(const QString& filePath, const VaultDocument& doc, QString* outError) {
    QFileInfo fileInfo(filePath);
    QDir().mkpath(fileInfo.absolutePath());

    // Lock file to protect against concurrent writes
    QString lockPath = filePath + ".lock";
    QLockFile lock(lockPath);
    lock.setStaleLockTime(5000);
    if (!lock.lock()) {
        if (outError) {
            *outError = QStringLiteral("Bestand is momenteel vergrendeld door een ander proces.");
        }
        return false;
    }

    // 1. Create backup of current file if it exists
    if (QFile::exists(filePath)) {
        QString backupDir = getBackupDirectory();
        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_hhmmss");
        QString backupFile = QString("%1/vault_backup_%2.json").arg(backupDir, timestamp);
        QFile::copy(filePath, backupFile);

        // Prune older backups
        QDir bDir(backupDir);
        QStringList backupList = bDir.entryList(QStringList() << "vault_backup_*.json", QDir::Files, QDir::Time);
        while (backupList.size() > MAX_BACKUPS) {
            QString oldest = backupList.takeLast();
            bDir.remove(oldest);
        }
    }

    // 2. Write to temporary file
    QString tempPath = filePath + ".tmp";
    QFile tempFile(tempPath);
    if (!tempFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (outError) {
            *outError = QStringLiteral("Kan tijdelijk bestand niet aanmaken: ") + tempFile.errorString();
        }
        lock.unlock();
        return false;
    }

    QByteArray data = doc.toJsonData();
    if (tempFile.write(data) != data.size()) {
        if (outError) {
            *outError = QStringLiteral("Fout bij schrijven naar tijdelijk kluisbestand.");
        }
        tempFile.close();
        QFile::remove(tempPath);
        lock.unlock();
        return false;
    }

    tempFile.flush();
    tempFile.close();

    // 3. Atomic rename/replace
    if (QFile::exists(filePath)) {
        QFile::remove(filePath);
    }

    if (!tempFile.rename(filePath)) {
        // Fallback copy if rename fails
        if (!QFile::copy(tempPath, filePath)) {
            if (outError) {
                *outError = QStringLiteral("Kan kluisbestand niet bijwerken vanaf tijdelijk bestand.");
            }
            QFile::remove(tempPath);
            lock.unlock();
            return false;
        }
        QFile::remove(tempPath);
    }

    lock.unlock();
    return true;
}

QStringList VaultStorage::listBackups() {
    QString backupDir = getBackupDirectory();
    QDir bDir(backupDir);
    return bDir.entryList(QStringList() << "vault_backup_*.json", QDir::Files, QDir::Time);
}

bool VaultStorage::restoreBackup(const QString& backupFileName, const QString& targetVaultPath, QString* outError) {
    QString backupDir = getBackupDirectory();
    QString fullBackupPath = backupDir + "/" + backupFileName;
    if (!QFile::exists(fullBackupPath)) {
        if (outError) *outError = QStringLiteral("Back-up bestand niet gevonden.");
        return false;
    }

    VaultDocument doc = loadVault(fullBackupPath, outError);
    return saveVault(targetVaultPath, doc, outError);
}

bool VaultStorage::migrateVault(const QString& sourcePath, const QString& destinationPath, QString* outError) {
    if (sourcePath == destinationPath) {
        return true;
    }

    VaultDocument doc = loadVault(sourcePath, outError);
    if (!saveVault(destinationPath, doc, outError)) {
        return false;
    }

    return true;
}

} // namespace core
