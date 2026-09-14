#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include "core/VaultModel.h"

namespace core {

class VaultStorage {
public:
    // Returns default vault file path based on detected providers (OneDrive first, then Google Drive, then local)
    static QString getDefaultVaultPath();

    // Provider specific paths
    static QString getOneDriveDirectory();
    static QString getOneDriveVaultPath();

    static QString getGoogleDriveDirectory();
    static QString getGoogleDriveVaultPath();

    static QString getLocalVaultPath();

    // Resolves path based on provider preference
    static QString resolveVaultPath(CloudProvider provider, const QString& customPath = QString());

    // Checks if cloud provider path is detected and accessible on current system
    static bool isCloudPathAvailable(CloudProvider provider);

    // Returns local backup directory path
    static QString getBackupDirectory();

    // Loads vault with shared-read access
    static VaultDocument loadVault(const QString& filePath, QString* outError = nullptr);

    // Saves vault atomically with lock file and automatic local backup creation
    static bool saveVault(const QString& filePath, const VaultDocument& doc, QString* outError = nullptr);

    // Lists existing backup files (sorted newest first)
    static QStringList listBackups();

    // Restores a backup
    static bool restoreBackup(const QString& backupFilePath, const QString& targetVaultPath, QString* outError = nullptr);

    // Migrates vault data from one path to another
    static bool migrateVault(const QString& sourcePath, const QString& destinationPath, QString* outError = nullptr);
};

} // namespace core
