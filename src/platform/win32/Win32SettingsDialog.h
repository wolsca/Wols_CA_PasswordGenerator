#pragma once
#include "core/PasswordOptions.h"
#include <windows.h>

namespace win32 {

class Win32SettingsDialog {
public:
    static bool show(HWND hParent, core::PasswordOptions& options);

private:
    static INT_PTR CALLBACK DialogProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    static void updateControlStates(HWND hDlg);
    static void loadValues(HWND hDlg, const core::PasswordOptions& options);
    static bool saveValues(HWND hDlg, core::PasswordOptions& options);
};

} // namespace win32
