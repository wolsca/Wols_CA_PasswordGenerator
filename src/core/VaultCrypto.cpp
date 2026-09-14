#include "core/VaultCrypto.h"
#include "core/SecureRandom.h"
#include <QPasswordDigestor>
#include <QCryptographicHash>
#include <QMessageAuthenticationCode>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS ((NTSTATUS)0x00000000L)
#endif
#endif

namespace core {

QString VaultCrypto::generateSalt() {
    uint8_t saltBytes[16];
    SecureRandom::getBytes(saltBytes, sizeof(saltBytes));
    return QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(saltBytes), sizeof(saltBytes)).toBase64());
}

QByteArray VaultCrypto::deriveKey(const QString& masterPassword, const QString& base64Salt, int iterations) {
    if (masterPassword.isEmpty()) return QByteArray();
    QByteArray salt = QByteArray::fromBase64(base64Salt.toLatin1());
    if (salt.isEmpty()) {
        salt = QByteArray(16, 0);
    }
    if (iterations < 1000) {
        iterations = 100000;
    }
    return QPasswordDigestor::deriveKeyPbkdf2(QCryptographicHash::Sha256, masterPassword.toUtf8(), salt, iterations, 32);
}

#ifdef _WIN32

bool VaultCrypto::encryptPassword(const QString& plaintext, 
                                  const QByteArray& key, 
                                  QString& outCiphertextBase64, 
                                  QString& outNonceBase64, 
                                  QString& outAuthTagBase64) 
{
    if (key.size() != 32) return false;

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (status != STATUS_SUCCESS) return false;

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, 
                               (PUCHAR)BCRYPT_CHAIN_MODE_GCM, 
                               sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (status != STATUS_SUCCESS) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_KEY_HANDLE hKey = nullptr;
    status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, 
                                        (PUCHAR)key.constData(), (ULONG)key.size(), 0);
    if (status != STATUS_SUCCESS) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    // 12-byte nonce for AES-GCM
    uint8_t nonce[12];
    SecureRandom::getBytes(nonce, sizeof(nonce));

    uint8_t tag[16] = {0};
    QByteArray plainUtf8 = plaintext.toUtf8();
    ULONG cipherSize = (ULONG)plainUtf8.size();
    QByteArray cipherBytes(cipherSize, 0);

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = nonce;
    authInfo.cbNonce = sizeof(nonce);
    authInfo.pbTag = tag;
    authInfo.cbTag = sizeof(tag);

    ULONG resultLen = 0;
    status = BCryptEncrypt(hKey, 
                           (PUCHAR)plainUtf8.constData(), (ULONG)plainUtf8.size(), 
                           &authInfo, 
                           nullptr, 0, 
                           (PUCHAR)cipherBytes.data(), (ULONG)cipherBytes.size(), 
                           &resultLen, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != STATUS_SUCCESS) {
        return false;
    }

    outCiphertextBase64 = QString::fromLatin1(cipherBytes.toBase64());
    outNonceBase64 = QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(nonce), sizeof(nonce)).toBase64());
    outAuthTagBase64 = QString::fromLatin1(QByteArray(reinterpret_cast<const char*>(tag), sizeof(tag)).toBase64());
    return true;
}

bool VaultCrypto::decryptPassword(const QString& ciphertextBase64, 
                                  const QString& nonceBase64, 
                                  const QString& authTagBase64, 
                                  const QByteArray& key, 
                                  QString& outPlaintext) 
{
    if (key.size() != 32) return false;

    QByteArray cipherBytes = QByteArray::fromBase64(ciphertextBase64.toLatin1());
    QByteArray nonceBytes = QByteArray::fromBase64(nonceBase64.toLatin1());
    QByteArray tagBytes = QByteArray::fromBase64(authTagBase64.toLatin1());

    if (nonceBytes.size() != 12 || tagBytes.size() != 16) {
        return false;
    }

    BCRYPT_ALG_HANDLE hAlg = nullptr;
    NTSTATUS status = BCryptOpenAlgorithmProvider(&hAlg, BCRYPT_AES_ALGORITHM, nullptr, 0);
    if (status != STATUS_SUCCESS) return false;

    status = BCryptSetProperty(hAlg, BCRYPT_CHAINING_MODE, 
                               (PUCHAR)BCRYPT_CHAIN_MODE_GCM, 
                               sizeof(BCRYPT_CHAIN_MODE_GCM), 0);
    if (status != STATUS_SUCCESS) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_KEY_HANDLE hKey = nullptr;
    status = BCryptGenerateSymmetricKey(hAlg, &hKey, nullptr, 0, 
                                        (PUCHAR)key.constData(), (ULONG)key.size(), 0);
    if (status != STATUS_SUCCESS) {
        BCryptCloseAlgorithmProvider(hAlg, 0);
        return false;
    }

    BCRYPT_AUTHENTICATED_CIPHER_MODE_INFO authInfo;
    BCRYPT_INIT_AUTH_MODE_INFO(authInfo);
    authInfo.pbNonce = (PUCHAR)nonceBytes.data();
    authInfo.cbNonce = (ULONG)nonceBytes.size();
    authInfo.pbTag = (PUCHAR)tagBytes.data();
    authInfo.cbTag = (ULONG)tagBytes.size();

    QByteArray plainBytes(cipherBytes.size(), 0);
    ULONG resultLen = 0;

    status = BCryptDecrypt(hKey, 
                           (PUCHAR)cipherBytes.constData(), (ULONG)cipherBytes.size(), 
                           &authInfo, 
                           nullptr, 0, 
                           (PUCHAR)plainBytes.data(), (ULONG)plainBytes.size(), 
                           &resultLen, 0);

    BCryptDestroyKey(hKey);
    BCryptCloseAlgorithmProvider(hAlg, 0);

    if (status != STATUS_SUCCESS) {
        return false;
    }

    plainBytes.resize(resultLen);
    outPlaintext = QString::fromUtf8(plainBytes);
    return true;
}

#else

// Cross-platform fallback implementation using HMAC-authenticated keystream / XOR CTR mode
bool VaultCrypto::encryptPassword(const QString& plaintext, 
                                  const QByteArray& key, 
                                  QString& outCiphertextBase64, 
                                  QString& outNonceBase64, 
                                  QString& outAuthTagBase64) 
{
    if (key.size() != 32) return false;
    uint8_t nonce[16];
    SecureRandom::getBytes(nonce, sizeof(nonce));
    QByteArray nonceBytes(reinterpret_cast<const char*>(nonce), sizeof(nonce));

    QByteArray plainBytes = plaintext.toUtf8();
    QByteArray cipherBytes(plainBytes.size(), 0);

    // Keystream generation via HMAC-SHA256
    int blocks = (plainBytes.size() + 31) / 32;
    if (blocks == 0) blocks = 1;
    QByteArray keystream;
    for (int b = 0; b < blocks; ++b) {
        QByteArray counterData = nonceBytes + QByteArray::number(b);
        keystream.append(QMessageAuthenticationCode::hash(counterData, key, QCryptographicHash::Sha256));
    }

    for (int i = 0; i < plainBytes.size(); ++i) {
        cipherBytes[i] = plainBytes[i] ^ keystream[i];
    }

    QByteArray tag = QMessageAuthenticationCode::hash(nonceBytes + cipherBytes, key, QCryptographicHash::Sha256).left(16);

    outCiphertextBase64 = QString::fromLatin1(cipherBytes.toBase64());
    outNonceBase64 = QString::fromLatin1(nonceBytes.toBase64());
    outAuthTagBase64 = QString::fromLatin1(tag.toBase64());
    return true;
}

bool VaultCrypto::decryptPassword(const QString& ciphertextBase64, 
                                  const QString& nonceBase64, 
                                  const QString& authTagBase64, 
                                  const QByteArray& key, 
                                  QString& outPlaintext) 
{
    if (key.size() != 32) return false;
    QByteArray cipherBytes = QByteArray::fromBase64(ciphertextBase64.toLatin1());
    QByteArray nonceBytes = QByteArray::fromBase64(nonceBase64.toLatin1());
    QByteArray tagBytes = QByteArray::fromBase64(authTagBase64.toLatin1());

    QByteArray expectedTag = QMessageAuthenticationCode::hash(nonceBytes + cipherBytes, key, QCryptographicHash::Sha256).left(16);
    if (expectedTag != tagBytes) {
        return false;
    }

    int blocks = (cipherBytes.size() + 31) / 32;
    if (blocks == 0) blocks = 1;
    QByteArray keystream;
    for (int b = 0; b < blocks; ++b) {
        QByteArray counterData = nonceBytes + QByteArray::number(b);
        keystream.append(QMessageAuthenticationCode::hash(counterData, key, QCryptographicHash::Sha256));
    }

    QByteArray plainBytes(cipherBytes.size(), 0);
    for (int i = 0; i < cipherBytes.size(); ++i) {
        plainBytes[i] = cipherBytes[i] ^ keystream[i];
    }

    outPlaintext = QString::fromUtf8(plainBytes);
    return true;
}

#endif

} // namespace core
