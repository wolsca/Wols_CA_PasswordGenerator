#pragma once

#include <QString>
#include <QByteArray>
#include "core/VaultModel.h"

namespace core {

class VaultCrypto {
public:
    // Derives a 256-bit (32 bytes) master key using PBKDF2-HMAC-SHA256
    static QByteArray deriveKey(const QString& masterPassword, const QString& base64Salt, int iterations = 100000);

    // Encrypts plaintext using AES-256-GCM (or Authenticated AES)
    static bool encryptPassword(const QString& plaintext, 
                                const QByteArray& key, 
                                QString& outCiphertextBase64, 
                                QString& outNonceBase64, 
                                QString& outAuthTagBase64);

    // Decrypts ciphertext using AES-256-GCM
    static bool decryptPassword(const QString& ciphertextBase64, 
                                const QString& nonceBase64, 
                                const QString& authTagBase64, 
                                const QByteArray& key, 
                                QString& outPlaintext);

    // Generates a new random Base64 salt (16 bytes)
    static QString generateSalt();
};

} // namespace core
