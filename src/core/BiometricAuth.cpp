#include "core/BiometricAuth.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <wincred.h>
#include <objbase.h>

namespace core {

bool BiometricAuth::isAvailable() {
    return true;
}

bool BiometricAuth::authenticate(const QString& reason, QWidget* parent) {
    HWND hwndParent = parent ? reinterpret_cast<HWND>(parent->winId()) : NULL;

    CREDUI_INFOW credUiInfo;
    ZeroMemory(&credUiInfo, sizeof(credUiInfo));
    credUiInfo.cbSize = sizeof(credUiInfo);
    credUiInfo.hwndParent = hwndParent;

    std::wstring message = reason.toStdWString();
    credUiInfo.pszMessageText = message.c_str();
    credUiInfo.pszCaptionText = L"Wols Password Manager - Verificatie";

    ULONG authPackage = 0;
    LPVOID outCredBuffer = nullptr;
    ULONG outCredSize = 0;
    BOOL save = FALSE;

    DWORD dwFlags = CREDUIWIN_GENERIC;

    DWORD dwErr = CredUIPromptForWindowsCredentialsW(
        &credUiInfo,
        0,
        &authPackage,
        nullptr,
        0,
        &outCredBuffer,
        &outCredSize,
        &save,
        dwFlags
    );

    if (dwErr == ERROR_SUCCESS) {
        if (outCredBuffer) {
            SecureZeroMemory(outCredBuffer, outCredSize);
            CoTaskMemFree(outCredBuffer);
        }
        return true;
    }

    return false;
}

} // namespace core

#else

namespace core {

bool BiometricAuth::isAvailable() {
    return false;
}

bool BiometricAuth::authenticate(const QString& /*reason*/, QWidget* /*parent*/) {
    return true; // Non-blocking on non-supported platforms
}

} // namespace core

#endif
