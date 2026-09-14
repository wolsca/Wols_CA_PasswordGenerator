#pragma once

#include <QString>
#include <QStringList>
#include <QDateTime>
#include "core/VaultModel.h"

namespace core {

class VaultStorage {
public:
    // Local configuration in OS user directory
    static QString getLocalConfigDirectory();
    static QString getLocalConfigPath();

    // Saves local configuration JSON to the OS user directory
    static bool saveLocalConfig(const QString& activeVaultPath, CloudProvider provider, const QString& selectedOneDriveDir, const VaultSettings& settings, QString* outError = nullptr);

    // Loads local configuration JSON from the OS user directory
    static bool loadLocalConfig(QString& outVaultPath, CloudProvider& outProvider, QString& outSelectedOneDriveDir, VaultSettings& outSettings, QString* outError = nullptr);

    // Generates cross-platform JSON config string for Windows, Linux, Android, or All
    static QString generatePlatformConfigJsonString(const QString& targetPlatform, const QString& activeVaultPath, CloudProvider provider, const QString& selectedOneDriveDir, const VaultSettings& settings);

    // Returns all detected OneDrive directories on the system
    static QStringList getAvailableOneDriveDirectories();

    // Returns all detected Google Drive directories on the system
    static QStringList getAvailableGoogleDriveDirectories();

    // Scans known locations for an existing, valid vault.json file
    static QString findExistingVault(CloudProvider* outProvider = nullptr, QString* outOneDriveDir = nullptr);

    // Returns default vault file path based on detected providers (OneDrive first, then Google Drive, then local)
    static QString getDefaultVaultPath(const QString& preferredOneDriveDir = QString());

    // Provider specific paths
    static QString getOneDriveDirectory(const QString& preferredDir = QString());
    static QString getOneDriveVaultPath(const QString& preferredDir = QString());

    static QString getGoogleDriveDirectory();
    static QString getGoogleDriveVaultPath();

    static QString getLocalVaultPath();

    // Resolves path based on provider preference
    static QString resolveVaultPath(CloudProvider provider, const QString& customPath = QString(), const QString& preferredOneDriveDir = QString());

    // Checks if cloud provider path is detected and accessible on current system
    static bool isCloudPathAvailable(CloudProvider provider, const QString& preferredOneDriveDir = QString());

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
