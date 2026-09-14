#include "core/VaultModel.h"
#include "core/VaultCrypto.h"
#include "core/VaultStorage.h"
#include "core/BiometricAuth.h"
#include "core/ChromeImporter.h"
#include <cassert>
#include <iostream>
#include <QFile>
#include <QDir>
#include <QUuid>

void testVaultModel() {
    std::cout << "[TEST] Testing VaultModel serialization and default email..." << std::endl;

    core::VaultDocument doc = core::VaultDocument::createDefault();
    assert(doc.version == 1);
    assert(!doc.groups.isEmpty());

    // Test settings with biometrics and cloud provider
    doc.settings.requireBiometricsForReveal = true;
    doc.settings.requireBiometricsForCopy = true;
    doc.settings.requireBiometricsForVault = false;
    doc.settings.preferredProvider = core::CloudProvider::OneDrive;
    doc.settings.customVaultPath = "C:/Vaults/custom.json";

    // Create a group
    core::VaultGroup grp;
    grp.id = "grp_test";
    grp.name = "Social Media";

    // Create an item with multiple accounts
    core::VaultItem item;
    item.id = "item_google";
    item.title = "Google";
    item.url = "https://accounts.google.com";
    item.category = "E-mail";
    item.createdAt = QDateTime::currentDateTimeUtc();
    item.updatedAt = item.createdAt;
    item.lastAccessed = item.createdAt;

    core::AccountEntry acc1;
    acc1.id = "acc_1";
    acc1.label = "Personal";
    acc1.username = "oskar_personal";
    acc1.email = "oskar.personal@gmail.com";
    acc1.isDefaultEmail = true;
    acc1.lastAccessed = item.createdAt;

    core::AccountEntry acc2;
    acc2.id = "acc_2";
    acc2.label = "Work";
    acc2.username = "oskar_work";
    acc2.email = "oskar.work@company.com";
    acc2.isDefaultEmail = false;
    acc2.lastAccessed = item.createdAt;

    item.accounts.append(acc1);
    item.accounts.append(acc2);
    grp.items.append(item);
    doc.groups.append(grp);

    // Verify findDefaultAccount
    const core::AccountEntry* defAcc = item.findDefaultAccount();
    assert(defAcc != nullptr);
    assert(defAcc->id == "acc_1");
    assert(defAcc->isDefaultEmail == true);

    // Serialize to JSON and parse back
    QByteArray jsonData = doc.toJsonData();
    assert(!jsonData.isEmpty());

    bool ok = false;
    core::VaultDocument parsed = core::VaultDocument::fromJsonData(jsonData, &ok);
    assert(ok);
    assert(parsed.groups.size() == doc.groups.size());
    assert(parsed.settings.requireBiometricsForReveal == true);
    assert(parsed.settings.requireBiometricsForCopy == true);
    assert(parsed.settings.requireBiometricsForVault == false);
    assert(parsed.settings.preferredProvider == core::CloudProvider::OneDrive);
    assert(parsed.settings.customVaultPath == "C:/Vaults/custom.json");

    const core::VaultGroup* foundGrp = nullptr;
    for (const auto& g : parsed.groups) {
        if (g.id == "grp_test") {
            foundGrp = &g;
            break;
        }
    }
    assert(foundGrp != nullptr);
    assert(foundGrp->items.size() == 1);
    assert(foundGrp->items[0].accounts.size() == 2);
    assert(foundGrp->items[0].accounts[0].isDefaultEmail == true);
    assert(foundGrp->items[0].accounts[1].isDefaultEmail == false);
    assert(foundGrp->items[0].accounts[0].email == "oskar.personal@gmail.com");

    std::cout << "  -> VaultModel serialization and default email passed!" << std::endl;
}

void testVaultCrypto() {
    std::cout << "[TEST] Testing VaultCrypto AES-GCM / PBKDF2 encryption..." << std::endl;

    QString masterPass = "SecretMasterPassword123!#";
    QString salt = core::VaultCrypto::generateSalt();
    assert(!salt.isEmpty());

    QByteArray key = core::VaultCrypto::deriveKey(masterPass, salt, 10000);
    assert(key.size() == 32);

    QString originalPassword = "MySuperSecretPassword@2026";
    QString ciphertext, nonce, authTag;

    bool encOk = core::VaultCrypto::encryptPassword(originalPassword, key, ciphertext, nonce, authTag);
    assert(encOk);
    assert(!ciphertext.isEmpty());
    assert(!nonce.isEmpty());
    assert(!authTag.isEmpty());

    // Decrypt with correct key
    QString decrypted;
    bool decOk = core::VaultCrypto::decryptPassword(ciphertext, nonce, authTag, key, decrypted);
    assert(decOk);
    assert(decrypted == originalPassword);

    // Decrypt with wrong key
    QByteArray wrongKey = core::VaultCrypto::deriveKey("WrongPassword999", salt, 10000);
    QString wrongDecrypted;
    bool wrongDecOk = core::VaultCrypto::decryptPassword(ciphertext, nonce, authTag, wrongKey, wrongDecrypted);
    assert(!wrongDecOk);

    // Tampered ciphertext
    QString tamperedCiphertext = ciphertext;
    tamperedCiphertext[0] = (tamperedCiphertext[0] == 'A') ? 'B' : 'A';
    bool tamperedOk = core::VaultCrypto::decryptPassword(tamperedCiphertext, nonce, authTag, key, decrypted);
    assert(!tamperedOk);

    std::cout << "  -> VaultCrypto AES-GCM / PBKDF2 passed!" << std::endl;
}

void testVaultStorage() {
    std::cout << "[TEST] Testing VaultStorage atomic saves, migrations, and cloud resolution..." << std::endl;

    // Test cloud provider resolution
    QString localPath = core::VaultStorage::getLocalVaultPath();
    assert(!localPath.isEmpty());
    assert(localPath.endsWith("vault.json"));

    QString defaultPath = core::VaultStorage::getDefaultVaultPath();
    assert(!defaultPath.isEmpty());

    QString customResolved = core::VaultStorage::resolveVaultPath(core::CloudProvider::Custom, "C:/MyPath/vault.json");
    assert(customResolved == "C:/MyPath/vault.json");

    assert(core::VaultStorage::isCloudPathAvailable(core::CloudProvider::Local));

    // Test saves and loads
    QString testVaultPath = QDir::tempPath() + "/test_vault_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".json";
    QString migrateVaultPath = QDir::tempPath() + "/test_vault_migrated_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".json";

    core::VaultDocument doc = core::VaultDocument::createDefault();
    doc.settings.lastGeneratorLength = 28;

    QString err;
    bool saveOk = core::VaultStorage::saveVault(testVaultPath, doc, &err);
    assert(saveOk);
    assert(QFile::exists(testVaultPath));

    // Load back
    core::VaultDocument loaded = core::VaultStorage::loadVault(testVaultPath, &err);
    assert(loaded.settings.lastGeneratorLength == 28);

    // Test migration
    bool migOk = core::VaultStorage::migrateVault(testVaultPath, migrateVaultPath, &err);
    assert(migOk);
    assert(QFile::exists(migrateVaultPath));
    core::VaultDocument loadedMig = core::VaultStorage::loadVault(migrateVaultPath, &err);
    assert(loadedMig.settings.lastGeneratorLength == 28);

    // Save again to trigger backup
    doc.settings.lastGeneratorLength = 36;
    saveOk = core::VaultStorage::saveVault(testVaultPath, doc, &err);
    assert(saveOk);

    core::VaultDocument loaded2 = core::VaultStorage::loadVault(testVaultPath, &err);
    assert(loaded2.settings.lastGeneratorLength == 36);

    // Cleanup test files
    QFile::remove(testVaultPath);
    QFile::remove(testVaultPath + ".lock");
    QFile::remove(testVaultPath + ".tmp");
    QFile::remove(migrateVaultPath);
    QFile::remove(migrateVaultPath + ".lock");
    QFile::remove(migrateVaultPath + ".tmp");

    std::cout << "  -> VaultStorage atomic saves, migrations, and cloud resolution passed!" << std::endl;
}

void testBiometricAuth() {
    std::cout << "[TEST] Testing BiometricAuth availability check..." << std::endl;
#ifdef _WIN32
    assert(core::BiometricAuth::isAvailable() == true);
#endif
    std::cout << "  -> BiometricAuth check passed!" << std::endl;
}

void testChromeImporter() {
    std::cout << "[TEST] Testing ChromeImporter CSV parser, encryption, and file wiping..." << std::endl;

    // 1. Test CSV Parser with quoted commas, escaped quotes, newlines
    QString sampleCsv = 
        "name,url,username,password,note\n"
        "\"Google, Inc.\",https://accounts.google.com,\"user@gmail.com\",\"pass\"\"123\",My Note\n"
        "GitHub,https://github.com/login,gituser,SuperSecret!,\n"
        "GitHub,https://github.com/login,gituser_work,WorkSecret!,\n";

    QVector<QStringList> parsed = core::ChromeImporter::parseCsv(sampleCsv);
    assert(parsed.size() == 4);
    assert(parsed[0].size() == 5);
    assert(parsed[1][0] == "Google, Inc.");
    assert(parsed[1][3] == "pass\"123");
    assert(parsed[1][4] == "My Note");

    // 2. Test Import and Encryption on the fly
    core::VaultDocument doc = core::VaultDocument::createDefault();
    QByteArray masterKey(32, 'K'); // Dummy 256-bit test key

    core::ChromeImportResult res = core::ChromeImporter::importFromCsvData(sampleCsv, masterKey, doc, "Geïmporteerd");
    assert(res.success);
    assert(res.importedCount == 3);

    // Verify imported group exists
    const core::VaultGroup* importGrp = nullptr;
    for (const auto& g : doc.groups) {
        if (g.name == "Geïmporteerd") {
            importGrp = &g;
            break;
        }
    }
    assert(importGrp != nullptr);
    assert(importGrp->items.size() == 2); // Google and GitHub (with 2 accounts)

    // Verify Google item and decrypted password
    const core::VaultItem* googleItem = nullptr;
    const core::VaultItem* githubItem = nullptr;
    for (const auto& itm : importGrp->items) {
        if (itm.title == "Google, Inc.") googleItem = &itm;
        if (itm.title == "GitHub") githubItem = &itm;
    }
    assert(googleItem != nullptr);
    assert(googleItem->accounts.size() == 1);
    assert(googleItem->accounts[0].email == "user@gmail.com");
    assert(googleItem->accounts[0].isDefaultEmail == true);

    QString decryptedPass;
    bool decOk = core::VaultCrypto::decryptPassword(googleItem->accounts[0].encryptedPassword, 
                                                    googleItem->accounts[0].nonce, 
                                                    googleItem->accounts[0].authTag, 
                                                    masterKey, 
                                                    decryptedPass);
    assert(decOk);
    assert(decryptedPass == "pass\"123");

    QString decryptedNote;
    bool decNoteOk = core::VaultCrypto::decryptPassword(googleItem->notesEncrypted,
                                                        googleItem->notesNonce,
                                                        googleItem->notesTag,
                                                        masterKey,
                                                        decryptedNote);
    assert(decNoteOk);
    assert(decryptedNote == "My Note");

    // Verify GitHub merged accounts
    assert(githubItem != nullptr);
    assert(githubItem->accounts.size() == 2);
    assert(githubItem->accounts[0].username == "gituser");
    assert(githubItem->accounts[1].username == "gituser_work");

    // 3. Test File Wiping (empty file instead of deleting)
    QString tempCsvPath = QDir::tempPath() + "/chrome_passwords_test_" + QUuid::createUuid().toString(QUuid::WithoutBraces) + ".csv";
    {
        QFile tf(tempCsvPath);
        bool openOk = tf.open(QIODevice::WriteOnly | QIODevice::Text);
        assert(openOk);
        tf.write(sampleCsv.toUtf8());
        tf.close();
    }

    assert(QFile::exists(tempCsvPath));
    assert(QFileInfo(tempCsvPath).size() > 0);

    // Wipe / Empty the file
    QString wipeErr;
    bool wipeOk = core::ChromeImporter::wipeFile(tempCsvPath, &wipeErr);
    assert(wipeOk);
    assert(QFile::exists(tempCsvPath)); // File still exists (not deleted)
    assert(QFileInfo(tempCsvPath).size() == 0); // But is now completely emptied (0 bytes)

    // Cleanup temp test file
    QFile::remove(tempCsvPath);

    std::cout << "  -> ChromeImporter CSV parser, encryption, and file wiping passed!" << std::endl;
}

void testLocalConfigAndPlatforms() {
    std::cout << "[TEST] Testing Local OS config JSON, multi-platform configs, and OneDrive detection..." << std::endl;

    QString configPath = core::VaultStorage::getLocalConfigPath();
    assert(!configPath.isEmpty());
    assert(configPath.endsWith("config.json"));

    // Test platform config string generation
    core::VaultSettings settings;
    settings.lastGeneratorLength = 24;
    settings.preferredProvider = core::CloudProvider::OneDrive;
    settings.selectedOneDriveDir = "C:/Users/Test/OneDrive - Personal";

    QString winJson = core::VaultStorage::generatePlatformConfigJsonString("windows", "C:/Vault/vault.json", core::CloudProvider::OneDrive, settings.selectedOneDriveDir, settings);
    assert(winJson.contains("Windows"));
    assert(winJson.contains("OneDrive - Personal"));
    assert(winJson.contains("active_vault_path"));

    QString linuxJson = core::VaultStorage::generatePlatformConfigJsonString("linux", "~/OneDrive/vault.json", core::CloudProvider::OneDrive, QString(), settings);
    assert(linuxJson.contains("Linux"));
    assert(linuxJson.contains("~/.config/wols_password_generator/config.json"));

    QString androidJson = core::VaultStorage::generatePlatformConfigJsonString("android", "/storage/emulated/0/Documents/vault.json", core::CloudProvider::OneDrive, QString(), settings);
    assert(androidJson.contains("Android"));
    assert(androidJson.contains("/storage/emulated/0/Documents/WolsPasswordManager/config.json"));

    QString allJson = core::VaultStorage::generatePlatformConfigJsonString("all", "C:/Vault/vault.json", core::CloudProvider::OneDrive, settings.selectedOneDriveDir, settings);
    assert(allJson.contains("windows"));
    assert(allJson.contains("linux"));
    assert(allJson.contains("android"));

    // Test saving and loading local config
    QString testVaultPath = "C:/TestLocation/vault.json";
    QString err;
    bool saveConfigOk = core::VaultStorage::saveLocalConfig(testVaultPath, core::CloudProvider::OneDrive, settings.selectedOneDriveDir, settings, &err);
    assert(saveConfigOk);
    assert(QFile::exists(configPath));

    QString loadedVaultPath;
    core::CloudProvider loadedProv = core::CloudProvider::Local;
    QString loadedSelectedOD;
    core::VaultSettings loadedSettings;
    bool loadConfigOk = core::VaultStorage::loadLocalConfig(loadedVaultPath, loadedProv, loadedSelectedOD, loadedSettings, &err);
    assert(loadConfigOk);
    assert(loadedVaultPath == testVaultPath);
    assert(loadedProv == core::CloudProvider::OneDrive);
    assert(loadedSelectedOD == "C:/Users/Test/OneDrive - Personal");
    assert(loadedSettings.lastGeneratorLength == 24);

    // Test OneDrive and Google Drive detection list
    QStringList availableOD = core::VaultStorage::getAvailableOneDriveDirectories();
    std::cout << "  -> Detected " << availableOD.size() << " OneDrive location(s) on current machine." << std::endl;

    QStringList availableGD = core::VaultStorage::getAvailableGoogleDriveDirectories();
    std::cout << "  -> Detected " << availableGD.size() << " Google Drive location(s) on current machine." << std::endl;

    // Test findExistingVault logic
    core::CloudProvider detectedProv;
    QString detectedODDir;
    QString existingVault = core::VaultStorage::findExistingVault(&detectedProv, &detectedODDir);
    std::cout << "  -> findExistingVault result: " << (existingVault.isEmpty() ? "None (clean system)" : existingVault.toStdString()) << std::endl;

    std::cout << "  -> Local OS config JSON, multi-platform configs, and Cloud detection passed!" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "========================================" << std::endl;
    std::cout << "   RUNNING VAULT UNIT TESTS             " << std::endl;
    std::cout << "========================================" << std::endl;

    testVaultModel();
    testVaultCrypto();
    testVaultStorage();
    testLocalConfigAndPlatforms();
    testBiometricAuth();
    testChromeImporter();

    std::cout << "========================================" << std::endl;
    std::cout << "   ALL VAULT TESTS PASSED!              " << std::endl;
    std::cout << "========================================" << std::endl;
    return 0;
}
