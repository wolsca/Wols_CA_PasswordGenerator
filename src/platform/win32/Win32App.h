#pragma once
#include "core/PasswordOptions.h"
#include "Win32Tray.h"
#include <windows.h>
#include <string>

namespace win32 {

class Win32App {
public:
    Win32App();
    ~Win32App();

    int run(HINSTANCE hInstance, int nCmdShow);

private:
    static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    void onCreate(HWND hWnd);
    void onCommand(HWND hWnd, WORD id, WORD code);
    void onHScroll(HWND hWnd, HWND hScrollWnd, int code, int pos);
    void onSize(HWND hWnd, UINT state, int cx, int cy);
    void onTrayMessage(HWND hWnd, WPARAM wParam, LPARAM lParam);
    void onDestroy(HWND hWnd);

    void generateAndDisplay();
    void copyToClipboard();
    void updateLengthFromSlider();
    void updateLengthFromEdit();
    void updateInfoDisplay();

    HWND m_hWnd = nullptr;
    HINSTANCE m_hInstance = nullptr;
    Win32Tray m_tray;

    // Controls
    HWND m_hEditPassword = nullptr;
    HWND m_hBtnRefresh = nullptr;
    HWND m_hBtnCopy = nullptr;
    HWND m_hBtnSettings = nullptr;
    HWND m_hSliderLength = nullptr;
    HWND m_hEditLength = nullptr;
    HWND m_hSpinLength = nullptr;
    HWND m_hLblLength = nullptr;
    HWND m_hLblInfo = nullptr;
    HWND m_hLblStatus = nullptr;

    HFONT m_hFontNormal = nullptr;
    HFONT m_hFontBold = nullptr;
    HFONT m_hFontPassword = nullptr;
    HFONT m_hFontIcon = nullptr;

    core::PasswordOptions m_options;
    std::string m_currentPassword;
    UINT_PTR m_timerId = 0;
};

} // namespace win32
