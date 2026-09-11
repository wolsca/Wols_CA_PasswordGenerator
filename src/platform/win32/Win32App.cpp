#include "Win32App.h"
#include "Win32SettingsDialog.h"
#include "core/PasswordGenerator.h"
#include "resource.h"
#include <commctrl.h>
#include <iomanip>
#include <sstream>
#include <vector>

#pragma comment(lib, "comctl32.lib")
#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#define WM_APP_TRAYMSG (WM_USER + 101)
#define IDT_RESET_STATUS 1001

namespace win32 {

Win32App::Win32App() {
    m_options.length = 16;
    m_options.useLetters = true;
    m_options.letterCase = core::LetterCase::Both;
    m_options.useDigits = true;
    m_options.useSpecialChars = true;
    m_options.specialCharSet = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
    m_options.hexOnly = false;
    m_options.hexCase = core::HexCase::Uppercase;
    m_options.useSignature = false;
    m_options.signaturePosition = 2;
}

Win32App::~Win32App() {
    if (m_hFontNormal) DeleteObject(m_hFontNormal);
    if (m_hFontBold) DeleteObject(m_hFontBold);
    if (m_hFontPassword) DeleteObject(m_hFontPassword);
    if (m_hFontIcon) DeleteObject(m_hFontIcon);
}

int Win32App::run(HINSTANCE hInstance, int nCmdShow) {
    m_hInstance = hInstance;

    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_BAR_CLASSES | ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t* CLASS_NAME = L"WolsPasswordGeneratorMainWindow";

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = MainWndProc;
    wc.hInstance = hInstance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszClassName = CLASS_NAME;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    if (!RegisterClassExW(&wc)) {
        return 0;
    }

    int winW = 540;
    int winH = 340;
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    int posX = (screenW - winW) / 2;
    int posY = (screenH - winH) / 2;

    m_hWnd = CreateWindowExW(
        WS_EX_APPWINDOW,
        CLASS_NAME,
        L"Password Generator",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        posX, posY, winW, winH,
        NULL, NULL, hInstance, this
    );

    if (!m_hWnd) return 0;

    ShowWindow(m_hWnd, nCmdShow);
    UpdateWindow(m_hWnd);

    // Initial password generation
    generateAndDisplay();

    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageW(m_hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    return (int)msg.wParam;
}

LRESULT CALLBACK Win32App::MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    Win32App* app = (Win32App*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

    switch (uMsg) {
    case WM_CREATE: {
        CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
        app = (Win32App*)cs->lpCreateParams;
        SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)app);
        if (app) app->onCreate(hWnd);
        return 0;
    }

    case WM_COMMAND: {
        if (app) {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);
            app->onCommand(hWnd, id, code);
        }
        return 0;
    }

    case WM_HSCROLL: {
        if (app) {
            app->onHScroll(hWnd, (HWND)lParam, LOWORD(wParam), HIWORD(wParam));
        }
        return 0;
    }

    case WM_SIZE: {
        if (app) {
            app->onSize(hWnd, (UINT)wParam, LOWORD(lParam), HIWORD(lParam));
        }
        return 0;
    }

    case WM_APP_TRAYMSG: {
        if (app) {
            app->onTrayMessage(hWnd, wParam, lParam);
        }
        return 0;
    }

    case WM_TIMER: {
        if (wParam == IDT_RESET_STATUS && app) {
            KillTimer(hWnd, IDT_RESET_STATUS);
            app->m_timerId = 0;
            SetWindowTextW(app->m_hLblStatus, L"Ready");
        }
        return 0;
    }

    case WM_DESTROY: {
        if (app) {
            app->onDestroy(hWnd);
        }
        PostQuitMessage(0);
        return 0;
    }
    }

    return DefWindowProcW(hWnd, uMsg, wParam, lParam);
}

void Win32App::onCreate(HWND hWnd) {
    m_hWnd = hWnd;

    // Fonts
    m_hFontNormal = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    m_hFontBold = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

    m_hFontPassword = CreateFontW(-17, 0, 0, 0, FW_SEMIBOLD, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  CLEARTYPE_QUALITY, FIXED_PITCH | FF_MODERN, L"Consolas");

    m_hFontIcon = CreateFontW(-15, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                              DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                              CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI Symbol");

    int x = 20;
    int y = 20;

    // Password Edit Box
    m_hEditPassword = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"",
        WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | ES_READONLY | WS_TABSTOP,
        x, y, 485, 36, hWnd, (HMENU)IDC_MAIN_PASSWORD_EDIT, m_hInstance, NULL
    );
    SendMessage(m_hEditPassword, WM_SETFONT, (WPARAM)m_hFontPassword, TRUE);

    // Action Buttons row
    y += 48;
    
    // Refresh (Generate) button with icon
    m_hBtnRefresh = CreateWindowExW(
        0, L"BUTTON", L"🔄 Generate",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x, y, 130, 34, hWnd, (HMENU)IDC_MAIN_BTN_REFRESH, m_hInstance, NULL
    );
    SendMessage(m_hBtnRefresh, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Copy button with icon
    m_hBtnCopy = CreateWindowExW(
        0, L"BUTTON", L"📋 Copy",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x + 140, y, 110, 34, hWnd, (HMENU)IDC_MAIN_BTN_COPY, m_hInstance, NULL
    );
    SendMessage(m_hBtnCopy, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Settings button
    m_hBtnSettings = CreateWindowExW(
        0, L"BUTTON", L"⚙ Settings...",
        WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
        x + 365, y, 120, 34, hWnd, (HMENU)IDC_MAIN_BTN_SETTINGS, m_hInstance, NULL
    );
    SendMessage(m_hBtnSettings, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // Separator line / Length Group
    y += 50;
    HWND hGrpLength = CreateWindowExW(
        0, L"BUTTON", L"Password Length",
        WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
        x, y, 485, 80, hWnd, NULL, m_hInstance, NULL
    );
    SendMessage(hGrpLength, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    // Length Slider (Trackbar)
    int sliderY = y + 28;
    m_hSliderLength = CreateWindowExW(
        0, TRACKBAR_CLASSW, L"",
        WS_CHILD | WS_VISIBLE | TBS_HORZ | TBS_AUTOTICKS | WS_TABSTOP,
        x + 15, sliderY, 340, 30, hWnd, (HMENU)IDC_MAIN_SLIDER_LENGTH, m_hInstance, NULL
    );
    SendMessage(m_hSliderLength, TBM_SETRANGE, TRUE, MAKELPARAM(8, 128));
    SendMessage(m_hSliderLength, TBM_SETPAGESIZE, 0, 8);
    SendMessage(m_hSliderLength, TBM_SETTICFREQ, 8, 0);
    SendMessage(m_hSliderLength, TBM_SETPOS, TRUE, m_options.length);

    // Length Edit & Spinner
    m_hEditLength = CreateWindowExW(
        WS_EX_CLIENTEDGE, L"EDIT", L"16",
        WS_CHILD | WS_VISIBLE | ES_NUMBER | ES_CENTER | WS_TABSTOP,
        x + 370, sliderY + 2, 55, 24, hWnd, (HMENU)IDC_MAIN_EDIT_LENGTH, m_hInstance, NULL
    );
    SendMessage(m_hEditLength, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    m_hSpinLength = CreateWindowExW(
        0, UPDOWN_CLASSW, NULL,
        WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
        0, 0, 0, 0, hWnd, NULL, m_hInstance, NULL
    );
    SendMessage(m_hSpinLength, UDM_SETBUDDY, (WPARAM)m_hEditLength, 0);
    SendMessage(m_hSpinLength, UDM_SETRANGE32, 8, 128);
    SendMessage(m_hSpinLength, UDM_SETPOS32, 0, m_options.length);

    // Info Label (Charset summary & Entropy)
    y += 92;
    m_hLblInfo = CreateWindowExW(
        0, L"STATIC", L"",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + 5, y, 475, 20, hWnd, (HMENU)IDC_MAIN_LABEL_INFO, m_hInstance, NULL
    );
    SendMessage(m_hLblInfo, WM_SETFONT, (WPARAM)m_hFontNormal, TRUE);

    // Status Label (e.g. Copied to clipboard!)
    y += 24;
    m_hLblStatus = CreateWindowExW(
        0, L"STATIC", L"Ready",
        WS_CHILD | WS_VISIBLE | SS_LEFT,
        x + 5, y, 475, 20, hWnd, (HMENU)IDC_MAIN_LABEL_STATUS, m_hInstance, NULL
    );
    SendMessage(m_hLblStatus, WM_SETFONT, (WPARAM)m_hFontBold, TRUE);

    updateInfoDisplay();
}

void Win32App::onCommand(HWND hWnd, WORD id, WORD code) {
    switch (id) {
    case IDC_MAIN_BTN_REFRESH:
        generateAndDisplay();
        break;

    case IDC_MAIN_BTN_COPY:
        copyToClipboard();
        break;

    case IDC_MAIN_BTN_SETTINGS: {
        core::PasswordOptions tempOpt = m_options;
        if (Win32SettingsDialog::show(hWnd, tempOpt)) {
            m_options = tempOpt;
            SendMessage(m_hSliderLength, TBM_SETPOS, TRUE, m_options.length);
            SendMessage(m_hSpinLength, UDM_SETPOS32, 0, m_options.length);
            generateAndDisplay();
        }
        break;
    }

    case IDC_MAIN_EDIT_LENGTH:
        if (code == EN_CHANGE) {
            updateLengthFromEdit();
        }
        break;

    case IDM_TRAY_SHOW:
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        m_tray.remove();
        break;

    case IDM_TRAY_GENERATE_COPY:
        generateAndDisplay();
        copyToClipboard();
        m_tray.showBalloon(L"Password Generator", L"New password generated and copied to clipboard!");
        break;

    case IDM_TRAY_EXIT:
        m_tray.remove();
        DestroyWindow(hWnd);
        break;
    }
}

void Win32App::onHScroll(HWND hWnd, HWND hScrollWnd, int code, int pos) {
    if (hScrollWnd == m_hSliderLength) {
        updateLengthFromSlider();
    }
}

void Win32App::onSize(HWND hWnd, UINT state, int cx, int cy) {
    if (state == SIZE_MINIMIZED) {
        // Minimize to System Tray
        ShowWindow(hWnd, SW_HIDE);
        m_tray.init(hWnd, WM_APP_TRAYMSG, LoadIcon(NULL, IDI_APPLICATION), L"Password Generator");
    }
}

void Win32App::onTrayMessage(HWND hWnd, WPARAM wParam, LPARAM lParam) {
    switch (LOWORD(lParam)) {
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case NIN_SELECT:
        ShowWindow(hWnd, SW_RESTORE);
        SetForegroundWindow(hWnd);
        m_tray.remove();
        break;

    case WM_RBUTTONUP:
    case WM_CONTEXTMENU:
        m_tray.showContextMenu(hWnd);
        break;
    }
}

void Win32App::onDestroy(HWND hWnd) {
    m_tray.remove();
}

void Win32App::updateLengthFromSlider() {
    int len = (int)SendMessage(m_hSliderLength, TBM_GETPOS, 0, 0);
    if (len != m_options.length) {
        m_options.length = len;
        SendMessage(m_hSpinLength, UDM_SETPOS32, 0, len);
        generateAndDisplay();
    }
}

void Win32App::updateLengthFromEdit() {
    wchar_t buf[32];
    GetWindowTextW(m_hEditLength, buf, 32);
    try {
        int len = std::stoi(buf);
        if (len >= 8 && len <= 128 && len != m_options.length) {
            m_options.length = len;
            SendMessage(m_hSliderLength, TBM_SETPOS, TRUE, len);
            generateAndDisplay();
        }
    } catch (...) {}
}

void Win32App::generateAndDisplay() {
    m_options.sanitize();
    m_currentPassword = core::PasswordGenerator::generate(m_options);

    std::wstring wPwd(m_currentPassword.begin(), m_currentPassword.end());
    SetWindowTextW(m_hEditPassword, wPwd.c_str());

    updateInfoDisplay();
}

void Win32App::copyToClipboard() {
    if (m_currentPassword.empty()) return;

    if (OpenClipboard(m_hWnd)) {
        EmptyClipboard();

        std::wstring wPwd(m_currentPassword.begin(), m_currentPassword.end());
        size_t sizeInBytes = (wPwd.length() + 1) * sizeof(wchar_t);

        HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, sizeInBytes);
        if (hGlob) {
            void* pGlob = GlobalLock(hGlob);
            if (pGlob) {
                memcpy(pGlob, wPwd.c_str(), sizeInBytes);
                GlobalUnlock(hGlob);
                SetClipboardData(CF_UNICODETEXT, hGlob);
            }
        }
        CloseClipboard();

        SetWindowTextW(m_hLblStatus, L"✓ Password copied to clipboard!");

        if (m_timerId != 0) {
            KillTimer(m_hWnd, IDT_RESET_STATUS);
        }
        m_timerId = SetTimer(m_hWnd, IDT_RESET_STATUS, 3000, NULL);
    }
}

void Win32App::updateInfoDisplay() {
    double entropy = core::PasswordGenerator::calculateEntropy(m_options);
    std::string summary = core::PasswordGenerator::getCharsetSummary(m_options);

    std::ostringstream ss;
    ss << "Entropy: ~" << static_cast<int>(entropy) << " bits";
    if (entropy < 50) ss << " (Weak)";
    else if (entropy < 80) ss << " (Good)";
    else if (entropy < 100) ss << " (Strong)";
    else ss << " (Very Strong)";

    ss << "  |  " << summary;

    std::string infoStr = ss.str();
    std::wstring wInfo(infoStr.begin(), infoStr.end());
    SetWindowTextW(m_hLblInfo, wInfo.c_str());
}

} // namespace win32
