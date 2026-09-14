#include "core/ChromeImporter.h"
#include "core/VaultCrypto.h"
#include "core/SecureRandom.h"
#include <QFile>
#include <QFileInfo>
#include <QUrl>
#include <QUuid>
#include <QDateTime>

namespace core {

QVector<QStringList> ChromeImporter::parseCsv(const QString& csvContent) {
    QVector<QStringList> records;
    QStringList currentRecord;
    QString currentField;
    bool inQuotes = false;

    const int length = csvContent.length();
    for (int i = 0; i < length; ++i) {
        const QChar c = csvContent.at(i);

        if (c == QLatin1Char('"')) {
            if (inQuotes && (i + 1 < length) && csvContent.at(i + 1) == QLatin1Char('"')) {
                // Escaped quote ("")
                currentField.append(QLatin1Char('"'));
                ++i;
            } else {
                inQuotes = !inQuotes;
            }
        } else if (c == QLatin1Char(',') && !inQuotes) {
            currentRecord.append(currentField);
            currentField.clear();
        } else if ((c == QLatin1Char('\r') || c == QLatin1Char('\n')) && !inQuotes) {
            if (c == QLatin1Char('\r') && (i + 1 < length) && csvContent.at(i + 1) == QLatin1Char('\n')) {
                ++i; // Skip \n after \r
            }
            currentRecord.append(currentField);
            currentField.clear();

            // Ignore empty trailing lines
            if (!currentRecord.isEmpty() && !(currentRecord.size() == 1 && currentRecord[0].isEmpty())) {
                records.append(currentRecord);
            }
            currentRecord.clear();
        } else {
            currentField.append(c);
        }
    }

    if (!currentField.isEmpty() || !currentRecord.isEmpty()) {
        currentRecord.append(currentField);
        if (!currentRecord.isEmpty() && !(currentRecord.size() == 1 && currentRecord[0].isEmpty())) {
            records.append(currentRecord);
        }
    }

    return records;
}

ChromeImportResult ChromeImporter::importFromCsvData(const QString& csvContent, 
                                                     const QByteArray& masterKey, 
                                                     VaultDocument& doc, 
                                                     const QString& targetGroupName) 
{
    ChromeImportResult result;
    if (masterKey.isEmpty()) {
        result.errors << QStringLiteral("Hoofdsleutel ontbreekt (ongeldige encryptiesleutel).");
        return result;
    }

    QVector<QStringList> rows = parseCsv(csvContent);
    if (rows.isEmpty()) {
        result.errors << QStringLiteral("Het CSV-bestand is leeg of bevat geen geldige rijen.");
        return result;
    }

    int nameCol = -1;
    int urlCol = -1;
    int userCol = -1;
    int passCol = -1;
    int noteCol = -1;

    int startRow = 0;

    // Check first row for Chrome / Standard headers
    const QStringList& header = rows[0];
    for (int col = 0; col < header.size(); ++col) {
        QString h = header[col].trimmed().toLower();
        if (h == QStringLiteral("name") || h == QStringLiteral("title") || h == QStringLiteral("site")) {
            nameCol = col;
        } else if (h == QStringLiteral("url") || h == QStringLiteral("web") || h == QStringLiteral("website")) {
            urlCol = col;
        } else if (h == QStringLiteral("username") || h == QStringLiteral("user") || h == QStringLiteral("login") || h == QStringLiteral("email")) {
            userCol = col;
        } else if (h == QStringLiteral("password") || h == QStringLiteral("pass") || h == QStringLiteral("pwd")) {
            passCol = col;
        } else if (h == QStringLiteral("note") || h == QStringLiteral("notes")) {
            noteCol = col;
        }
    }

    if (passCol != -1 && (userCol != -1 || urlCol != -1 || nameCol != -1)) {
        startRow = 1; // Header row detected
    } else {
        // Fallback to default Google Chrome CSV column ordering: name,url,username,password,note
        nameCol = 0;
        urlCol = 1;
        userCol = 2;
        passCol = 3;
        noteCol = 4;
        startRow = 0;
    }

    // Find or create the target group
    VaultGroup* targetGroup = nullptr;
    for (auto& grp : doc.groups) {
        if (grp.name.compare(targetGroupName, Qt::CaseInsensitive) == 0) {
            targetGroup = &grp;
            break;
        }
    }

    if (!targetGroup) {
        VaultGroup newGroup;
        newGroup.id = QStringLiteral("grp_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
        newGroup.name = targetGroupName.isEmpty() ? QStringLiteral("Geïmporteerd") : targetGroupName;
        doc.groups.append(newGroup);
        targetGroup = &doc.groups.last();
    }

    const QDateTime now = QDateTime::currentDateTimeUtc();

    for (int r = startRow; r < rows.size(); ++r) {
        const QStringList& row = rows[r];
        if (row.isEmpty()) continue;

        QString name = (nameCol >= 0 && nameCol < row.size()) ? row[nameCol].trimmed() : QString();
        QString url = (urlCol >= 0 && urlCol < row.size()) ? row[urlCol].trimmed() : QString();
        QString username = (userCol >= 0 && userCol < row.size()) ? row[userCol].trimmed() : QString();
        QString password = (passCol >= 0 && passCol < row.size()) ? row[passCol] : QString();
        QString note = (noteCol >= 0 && noteCol < row.size()) ? row[noteCol].trimmed() : QString();

        if (name.isEmpty() && url.isEmpty() && username.isEmpty() && password.isEmpty()) {
            continue; // Completely blank line
        }

        // Clean up title
        if (name.isEmpty()) {
            if (!url.isEmpty()) {
                QUrl parsedUrl(url);
                if (parsedUrl.isValid() && !parsedUrl.host().isEmpty()) {
                    name = parsedUrl.host();
                } else {
                    name = url;
                }
            } else if (!username.isEmpty()) {
                name = username;
            } else {
                name = QStringLiteral("Geïmporteerd Account %1").arg(r);
            }
        }

        // Search if item with same title or url already exists in this group
        VaultItem* targetItem = nullptr;
        for (auto& item : targetGroup->items) {
            if ((!url.isEmpty() && item.url.compare(url, Qt::CaseInsensitive) == 0) ||
                item.title.compare(name, Qt::CaseInsensitive) == 0) 
            {
                targetItem = &item;
                break;
            }
        }

        bool isNewItem = false;
        if (!targetItem) {
            VaultItem newItem;
            newItem.id = QStringLiteral("item_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
            newItem.title = name;
            newItem.url = url;
            newItem.createdAt = now;
            newItem.updatedAt = now;
            newItem.lastAccessed = now;

            if (!note.isEmpty()) {
                VaultCrypto::encryptPassword(note, masterKey, newItem.notesEncrypted, newItem.notesNonce, newItem.notesTag);
            }

            targetGroup->items.append(newItem);
            targetItem = &targetGroup->items.last();
            isNewItem = true;
        }

        // Search if account with same username exists in item
        AccountEntry* targetAccount = nullptr;
        if (!isNewItem) {
            for (auto& acc : targetItem->accounts) {
                if (acc.username.compare(username, Qt::CaseInsensitive) == 0 ||
                    acc.email.compare(username, Qt::CaseInsensitive) == 0) 
                {
                    targetAccount = &acc;
                    break;
                }
            }
        }

        if (targetAccount) {
            // Update existing account
            if (!password.isEmpty()) {
                VaultCrypto::encryptPassword(password, masterKey, targetAccount->encryptedPassword, targetAccount->nonce, targetAccount->authTag);
            }
            targetAccount->lastAccessed = now;
            targetItem->updatedAt = now;
            result.updatedCount++;
        } else {
            // Create new account
            AccountEntry newAcc;
            newAcc.id = QStringLiteral("acc_") + QUuid::createUuid().toString(QUuid::WithoutBraces);
            if (username.contains(QLatin1Char('@'))) {
                newAcc.email = username;
                newAcc.username = username;
                newAcc.isDefaultEmail = true;
            } else {
                newAcc.username = username;
                newAcc.isDefaultEmail = targetItem->accounts.isEmpty();
            }

            if (!password.isEmpty()) {
                VaultCrypto::encryptPassword(password, masterKey, newAcc.encryptedPassword, newAcc.nonce, newAcc.authTag);
            }
            newAcc.lastAccessed = now;

            targetItem->accounts.append(newAcc);
            targetItem->updatedAt = now;
            result.importedCount++;
        }
    }

    result.success = (result.importedCount > 0 || result.updatedCount > 0);
    return result;
}

ChromeImportResult ChromeImporter::importFromCsvFile(const QString& filePath, 
                                                     const QByteArray& masterKey, 
                                                     VaultDocument& doc, 
                                                     const QString& targetGroupName) 
{
    ChromeImportResult result;
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        result.errors << QString("Kon CSV-bestand niet openen: %1").arg(file.errorString());
        return result;
    }

    QByteArray data = file.readAll();
    file.close();

    QString content = QString::fromUtf8(data);
    return importFromCsvData(content, masterKey, doc, targetGroupName);
}

bool ChromeImporter::wipeFile(const QString& filePath, QString* outError) {
    QFileInfo fi(filePath);
    if (!fi.exists()) {
        if (outError) *outError = QStringLiteral("Bestand bestaat niet.");
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadWrite)) {
        if (outError) *outError = QString("Kan bestand niet openen voor overschrijven: %1").arg(file.errorString());
        return false;
    }

    qint64 size = file.size();
    if (size > 0) {
        // Pass 1: Cryptographically random data
        const qint64 chunkSize = 4096;
        qint64 remaining = size;
        file.seek(0);
        while (remaining > 0) {
            int currentChunk = static_cast<int>(qMin(remaining, chunkSize));
            QByteArray randBytes(currentChunk, 0);
            SecureRandom::getBytes(reinterpret_cast<uint8_t*>(randBytes.data()), currentChunk);
            file.write(randBytes);
            remaining -= currentChunk;
        }
        file.flush();

        // Pass 2: Overwrite with zeros
        remaining = size;
        file.seek(0);
        QByteArray zeroBytes(static_cast<int>(chunkSize), '\0');
        while (remaining > 0) {
            int currentChunk = static_cast<int>(qMin(remaining, chunkSize));
            file.write(zeroBytes.constData(), currentChunk);
            remaining -= currentChunk;
        }
        file.flush();
    }

    // Pass 3: Truncate / resize to 0 bytes so file is completely emptied
    if (!file.resize(0)) {
        if (outError) *outError = QString("Kan bestand niet verkleinen naar 0 bytes: %1").arg(file.errorString());
        file.close();
        return false;
    }

    file.flush();
    file.close();
    return true;
}

} // namespace core
