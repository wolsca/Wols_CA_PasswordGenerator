#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QByteArray>
#include "core/VaultModel.h"

namespace core {

struct ChromeImportResult {
    int importedCount = 0;
    int updatedCount = 0;
    int skippedCount = 0;
    QStringList errors;
    bool success = false;
};

class ChromeImporter {
public:
    // Parses a standard RFC 4180 CSV string into rows of columns
    static QVector<QStringList> parseCsv(const QString& csvContent);

    // Imports Google Chrome CSV contents directly into the VaultDocument and encrypts passwords on the fly
    static ChromeImportResult importFromCsvData(const QString& csvContent, 
                                               const QByteArray& masterKey, 
                                               VaultDocument& doc, 
                                               const QString& targetGroupName = QStringLiteral("Geïmporteerd"));

    // Reads a CSV file from disk and imports the passwords
    static ChromeImportResult importFromCsvFile(const QString& filePath, 
                                               const QByteArray& masterKey, 
                                               VaultDocument& doc, 
                                               const QString& targetGroupName = QStringLiteral("Geïmporteerd"));

    // Securely overwrites the file content with random & zero bytes and truncates/empties it to 0 bytes
    static bool wipeFile(const QString& filePath, QString* outError = nullptr);
};

} // namespace core
