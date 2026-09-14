#include "core/VaultStorage.h"
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QLockFile>
#include <QProcessEnvironment>
#include <algorithm>

namespace core {

namespace {
    const int MAX_BACKUPS = 10;
}

QString VaultStorage::getOneDriveDirectory() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

    // Check environment variables on Windows
    QStringList candidates;
    if (env.contains("OneDriveCommercial")) candidates << env.value("OneDriveCommercial");
    if (env.contains("OneDriveConsumer")) candidates << env.value("OneDriveConsumer");
    if (env.contains("OneDrive")) candidates << env.value("OneDrive");

    // Standard user profile paths
    QString userProfile = env.value("USERPROFILE");
    if (!userProfile.isEmpty()) {
        candidates << userProfile + "/OneDrive";
    }

    // Home path for cross-platform
    candidates << QDir::homePath() + "/OneDrive";

    for (const QString& path : candidates) {
        if (!path.isEmpty() && QDir(path).exists()) {
            // Check Documents / Documenten subfolder preferred
            if (QDir(path + "/Documents").exists()) return path + "/Documents";
            if (QDir(path + "/Documenten").exists()) return path + "/Documenten";
            return path;
        }
    }
    return QString();
}

QString VaultStorage::getOneDriveVaultPath() {
    QString oneDriveDir = getOneDriveDirectory();
    if (oneDriveDir.isEmpty()) {
        return QString();
    }
    QString folder = oneDriveDir + "/WolsPasswordManager";
    QDir().mkpath(folder);
    return folder + "/vault.json";
}

QString VaultStorage::getGoogleDriveDirectory() {
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QStringList candidates;

    // Check Windows mounted drive letters commonly used by Google Drive
    candidates << "G:/My Drive" << "G:/Mijn Drive" << "G:/"
               << "H:/My Drive" << "H:/Mijn Drive" << "H:/";

    // Check user profile paths
    QString userProfile = env.value("USERPROFILE");
    if (!userProfile.isEmpty()) {
        candidates << userProfile + "/Google Drive"
                   << userProfile + "/Google Drive/My Drive"
                   << userProfile + "/My Drive";
    }

    // Check home directory for Linux/macOS
    candidates << QDir::homePath() + "/Google Drive"
               << QDir::homePath() + "/GoogleDrive"
               << QDir::homePath() + "/My Drive";

    for (const QString& path : candidates) {
        if (!path.isEmpty() && QDir(path).exists()) {
            return path;
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

bool VaultStorage::isCloudPathAvailable(CloudProvider provider) {
    switch (provider) {
        case CloudProvider::OneDrive:
            return !getOneDriveDirectory().isEmpty();
        case CloudProvider::GoogleDrive:
            return !getGoogleDriveDirectory().isEmpty();
        case CloudProvider::Local:
            return true;
        case CloudProvider::Custom:
            return true;
    }
    return false;
}

QString VaultStorage::resolveVaultPath(CloudProvider provider, const QString& customPath) {
    switch (provider) {
        case CloudProvider::OneDrive: {
            QString path = getOneDriveVaultPath();
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
    return getDefaultVaultPath();
}

QString VaultStorage::getDefaultVaultPath() {
    // 1. Prioritize OneDrive if available
    QString oneDrivePath = getOneDriveVaultPath();
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
