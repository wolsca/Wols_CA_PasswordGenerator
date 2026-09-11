#include "Win32SettingsDialog.h"
#include "resource.h"
#include <commctrl.h>
#include <string>
#include <vector>

#pragma comment(lib, "comctl32.lib")

namespace win32 {

namespace {
    struct DialogContext {
        core::PasswordOptions* options = nullptr;
        bool result = false;
        HWND hParent = nullptr;
        HFONT hFont = nullptr;
        HFONT hBoldFont = nullptr;

        // Controls
        HWND hChkLetters = nullptr;
        HWND hRadBoth = nullptr;
        HWND hRadLower = nullptr;
        HWND hRadUpper = nullptr;

        HWND hChkDigits = nullptr;
        HWND hChkSpecial = nullptr;
        HWND hEditSpecial = nullptr;

        HWND hChkHex = nullptr;
        HWND hRadHexUpper = nullptr;
        HWND hRadHexLower = nullptr;

        HWND hChkSignature = nullptr;
        HWND hEditSigPos = nullptr;
        HWND hSpinSigPos = nullptr;
        HWND hLblSigDesc = nullptr;

        HWND hBtnOk = nullptr;
        HWND hBtnCancel = nullptr;
    };

    static DialogContext* g_ctx = nullptr;

    void updateUI(DialogContext* ctx) {
        if (!ctx) return;
        BOOL isHex = (SendMessage(ctx->hChkHex, BM_GETCHECK, 0, 0) == BST_CHECKED);
        
        EnableWindow(ctx->hRadHexUpper, isHex);
        EnableWindow(ctx->hRadHexLower, isHex);

        EnableWindow(ctx->hChkLetters, !isHex);
        BOOL lettersChecked = !isHex && (SendMessage(ctx->hChkLetters, BM_GETCHECK, 0, 0) == BST_CHECKED);
        EnableWindow(ctx->hRadBoth, lettersChecked);
        EnableWindow(ctx->hRadLower, lettersChecked);
        EnableWindow(ctx->hRadUpper, lettersChecked);

        EnableWindow(ctx->hChkDigits, !isHex);
        EnableWindow(ctx->hChkSpecial, !isHex);

        BOOL specialChecked = !isHex && (SendMessage(ctx->hChkSpecial, BM_GETCHECK, 0, 0) == BST_CHECKED);
        EnableWindow(ctx->hEditSpecial, specialChecked);

        BOOL sigChecked = (SendMessage(ctx->hChkSignature, BM_GETCHECK, 0, 0) == BST_CHECKED);
        EnableWindow(ctx->hEditSigPos, sigChecked);
        EnableWindow(ctx->hSpinSigPos, sigChecked);
    }

    LRESULT CALLBACK SettingsWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
        DialogContext* ctx = (DialogContext*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

        switch (uMsg) {
        case WM_CREATE: {
            CREATESTRUCTW* cs = (CREATESTRUCTW*)lParam;
            ctx = (DialogContext*)cs->lpCreateParams;
            SetWindowLongPtrW(hWnd, GWLP_USERDATA, (LONG_PTR)ctx);

            ctx->hFont = CreateFontW(-13, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                     DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                     CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");
            ctx->hBoldFont = CreateFontW(-13, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
                                         DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                         CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_DONTCARE, L"Segoe UI");

            int x = 20, y = 15;
            int groupW = 380;

            // Character Sets Group
            HWND hGrpChars = CreateWindowExW(0, L"BUTTON", L"Character Sets",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                x, y, groupW, 200, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hGrpChars, WM_SETFONT, (WPARAM)ctx->hBoldFont, TRUE);

            int iy = y + 25;
            ctx->hChkLetters = CreateWindowExW(0, L"BUTTON", L"Include Letters (A-Z, a-z)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                x + 15, iy, 250, 20, hWnd, (HMENU)IDC_SET_CHK_LETTERS, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hChkLetters, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 22;
            ctx->hRadBoth = CreateWindowExW(0, L"BUTTON", L"Both uppercase & lowercase",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
                x + 35, iy, 220, 20, hWnd, (HMENU)IDC_SET_RAD_BOTH, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hRadBoth, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 20;
            ctx->hRadLower = CreateWindowExW(0, L"BUTTON", L"Lowercase only (a-z)",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                x + 35, iy, 220, 20, hWnd, (HMENU)IDC_SET_RAD_LOWER, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hRadLower, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 20;
            ctx->hRadUpper = CreateWindowExW(0, L"BUTTON", L"Uppercase only (A-Z)",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                x + 35, iy, 220, 20, hWnd, (HMENU)IDC_SET_RAD_UPPER, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hRadUpper, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 24;
            ctx->hChkDigits = CreateWindowExW(0, L"BUTTON", L"Include Digits (0-9)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                x + 15, iy, 250, 20, hWnd, (HMENU)IDC_SET_CHK_DIGITS, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hChkDigits, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 24;
            ctx->hChkSpecial = CreateWindowExW(0, L"BUTTON", L"Include Special Characters:",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                x + 15, iy, 210, 20, hWnd, (HMENU)IDC_SET_CHK_SPECIAL, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hChkSpecial, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 22;
            ctx->hEditSpecial = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"",
                WS_CHILD | WS_VISIBLE | ES_AUTOHSCROLL | WS_TABSTOP,
                x + 35, iy, 320, 22, hWnd, (HMENU)IDC_SET_EDIT_SPECIAL, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hEditSpecial, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            // Hexadecimal Group
            y = 225;
            HWND hGrpHex = CreateWindowExW(0, L"BUTTON", L"Hexadecimal Mode",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                x, y, groupW, 80, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hGrpHex, WM_SETFONT, (WPARAM)ctx->hBoldFont, TRUE);

            iy = y + 22;
            ctx->hChkHex = CreateWindowExW(0, L"BUTTON", L"Hexadecimal Only (0-9, A-F)",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                x + 15, iy, 250, 20, hWnd, (HMENU)IDC_SET_CHK_HEX, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hChkHex, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 24;
            ctx->hRadHexUpper = CreateWindowExW(0, L"BUTTON", L"Uppercase (A-F)",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_GROUP | WS_TABSTOP,
                x + 35, iy, 140, 20, hWnd, (HMENU)IDC_SET_RAD_HEX_UPPER, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hRadHexUpper, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            ctx->hRadHexLower = CreateWindowExW(0, L"BUTTON", L"Lowercase (a-f)",
                WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON,
                x + 190, iy, 140, 20, hWnd, (HMENU)IDC_SET_RAD_HEX_LOWER, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hRadHexLower, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            // Signature Group
            y = 315;
            HWND hGrpSig = CreateWindowExW(0, L"BUTTON", L"Password Signature (Ownership Mark)",
                WS_CHILD | WS_VISIBLE | BS_GROUPBOX,
                x, y, groupW, 110, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hGrpSig, WM_SETFONT, (WPARAM)ctx->hBoldFont, TRUE);

            iy = y + 24;
            ctx->hChkSignature = CreateWindowExW(0, L"BUTTON", L"Enable Custom Signature Rule",
                WS_CHILD | WS_VISIBLE | BS_AUTOCHECKBOX | WS_TABSTOP,
                x + 15, iy, 250, 20, hWnd, (HMENU)IDC_SET_CHK_SIGNATURE, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hChkSignature, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            iy += 24;
            HWND hLblPos = CreateWindowExW(0, L"STATIC", L"Signature Position (1 to Length):",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                x + 35, iy + 2, 210, 20, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(hLblPos, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            ctx->hEditSigPos = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"2",
                WS_CHILD | WS_VISIBLE | ES_NUMBER | WS_TABSTOP,
                x + 250, iy, 50, 22, hWnd, (HMENU)IDC_SET_EDIT_SIG_POS, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hEditSigPos, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            ctx->hSpinSigPos = CreateWindowExW(0, UPDOWN_CLASSW, NULL,
                WS_CHILD | WS_VISIBLE | UDS_SETBUDDYINT | UDS_ALIGNRIGHT | UDS_ARROWKEYS | UDS_NOTHOUSANDS,
                0, 0, 0, 0, hWnd, (HMENU)IDC_SET_SPIN_SIG_POS, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hSpinSigPos, UDM_SETBUDDY, (WPARAM)ctx->hEditSigPos, 0);
            SendMessage(ctx->hSpinSigPos, UDM_SETRANGE32, 1, 128);

            iy += 26;
            ctx->hLblSigDesc = CreateWindowExW(0, L"STATIC",
                L"Guarantees a special character at this position;\r\nadjacent characters (1st & 3rd for pos 2) are non-special.",
                WS_CHILD | WS_VISIBLE | SS_LEFT,
                x + 35, iy, 330, 32, hWnd, NULL, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hLblSigDesc, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            // Bottom Buttons
            y = 440;
            ctx->hBtnOk = CreateWindowExW(0, L"BUTTON", L"Save",
                WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON | WS_TABSTOP,
                220, y, 85, 28, hWnd, (HMENU)IDC_SET_BTN_OK, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hBtnOk, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            ctx->hBtnCancel = CreateWindowExW(0, L"BUTTON", L"Cancel",
                WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
                315, y, 85, 28, hWnd, (HMENU)IDC_SET_BTN_CANCEL, GetModuleHandle(NULL), NULL);
            SendMessage(ctx->hBtnCancel, WM_SETFONT, (WPARAM)ctx->hFont, TRUE);

            // Fill initial values from options
            core::PasswordOptions& opt = *(ctx->options);
            SendMessage(ctx->hChkLetters, BM_SETCHECK, opt.useLetters ? BST_CHECKED : BST_UNCHECKED, 0);
            if (opt.letterCase == core::LetterCase::Both) {
                SendMessage(ctx->hRadBoth, BM_SETCHECK, BST_CHECKED, 0);
            } else if (opt.letterCase == core::LetterCase::LowerOnly) {
                SendMessage(ctx->hRadLower, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendMessage(ctx->hRadUpper, BM_SETCHECK, BST_CHECKED, 0);
            }

            SendMessage(ctx->hChkDigits, BM_SETCHECK, opt.useDigits ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(ctx->hChkSpecial, BM_SETCHECK, opt.useSpecialChars ? BST_CHECKED : BST_UNCHECKED, 0);
            
            std::wstring wSpecial(opt.specialCharSet.begin(), opt.specialCharSet.end());
            SetWindowTextW(ctx->hEditSpecial, wSpecial.c_str());

            SendMessage(ctx->hChkHex, BM_SETCHECK, opt.hexOnly ? BST_CHECKED : BST_UNCHECKED, 0);
            if (opt.hexCase == core::HexCase::Uppercase) {
                SendMessage(ctx->hRadHexUpper, BM_SETCHECK, BST_CHECKED, 0);
            } else {
                SendMessage(ctx->hRadHexLower, BM_SETCHECK, BST_CHECKED, 0);
            }

            SendMessage(ctx->hChkSignature, BM_SETCHECK, opt.useSignature ? BST_CHECKED : BST_UNCHECKED, 0);
            SendMessage(ctx->hSpinSigPos, UDM_SETPOS32, 0, opt.signaturePosition);

            updateUI(ctx);
            return 0;
        }

        case WM_COMMAND: {
            WORD id = LOWORD(wParam);
            WORD code = HIWORD(wParam);

            if (id == IDC_SET_CHK_HEX || id == IDC_SET_CHK_LETTERS || id == IDC_SET_CHK_SPECIAL || id == IDC_SET_CHK_SIGNATURE) {
                if (code == BN_CLICKED) {
                    updateUI(ctx);
                }
            } else if (id == IDC_SET_BTN_OK || id == IDOK) {
                // Save settings
                core::PasswordOptions& opt = *(ctx->options);
                opt.hexOnly = (SendMessage(ctx->hChkHex, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (SendMessage(ctx->hRadHexLower, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    opt.hexCase = core::HexCase::Lowercase;
                } else {
                    opt.hexCase = core::HexCase::Uppercase;
                }

                opt.useLetters = (SendMessage(ctx->hChkLetters, BM_GETCHECK, 0, 0) == BST_CHECKED);
                if (SendMessage(ctx->hRadLower, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    opt.letterCase = core::LetterCase::LowerOnly;
                } else if (SendMessage(ctx->hRadUpper, BM_GETCHECK, 0, 0) == BST_CHECKED) {
                    opt.letterCase = core::LetterCase::UpperOnly;
                } else {
                    opt.letterCase = core::LetterCase::Both;
                }

                opt.useDigits = (SendMessage(ctx->hChkDigits, BM_GETCHECK, 0, 0) == BST_CHECKED);
                opt.useSpecialChars = (SendMessage(ctx->hChkSpecial, BM_GETCHECK, 0, 0) == BST_CHECKED);

                wchar_t specBuf[256];
                GetWindowTextW(ctx->hEditSpecial, specBuf, 256);
                std::string specStr;
                for (int i = 0; specBuf[i] != 0; ++i) {
                    if (specBuf[i] < 128) specStr += static_cast<char>(specBuf[i]);
                }
                opt.specialCharSet = specStr.empty() ? "!@#$%^&*()_+-=[]{}|;:,.<>?/~" : specStr;

                opt.useSignature = (SendMessage(ctx->hChkSignature, BM_GETCHECK, 0, 0) == BST_CHECKED);
                opt.signaturePosition = (int)SendMessage(ctx->hSpinSigPos, UDM_GETPOS32, 0, 0);

                opt.sanitize();
                ctx->result = true;
                DestroyWindow(hWnd);
                return 0;
            } else if (id == IDC_SET_BTN_CANCEL || id == IDCANCEL) {
                ctx->result = false;
                DestroyWindow(hWnd);
                return 0;
            }
            break;
        }

        case WM_CLOSE:
            ctx->result = false;
            DestroyWindow(hWnd);
            return 0;

        case WM_DESTROY:
            if (ctx->hFont) DeleteObject(ctx->hFont);
            if (ctx->hBoldFont) DeleteObject(ctx->hBoldFont);
            return 0;
        }

        return DefWindowProcW(hWnd, uMsg, wParam, lParam);
    }
}

bool Win32SettingsDialog::show(HWND hParent, core::PasswordOptions& options) {
    INITCOMMONCONTROLSEX icex;
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_UPDOWN_CLASS | ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&icex);

    const wchar_t* CLASS_NAME = L"WolsSettingsDialogWnd";

    WNDCLASSEXW wc = { 0 };
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = SettingsWndProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_BTNFACE + 1);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    RegisterClassExW(&wc);

    DialogContext ctx;
    ctx.options = &options;
    ctx.hParent = hParent;
    ctx.result = false;
    g_ctx = &ctx;

    // Center over parent
    RECT rcParent;
    int dlgW = 440, dlgH = 520;
    int x = 100, y = 100;
    if (hParent && GetWindowRect(hParent, &rcParent)) {
        x = rcParent.left + (rcParent.right - rcParent.left - dlgW) / 2;
        y = rcParent.top + (rcParent.bottom - rcParent.top - dlgH) / 2;
    }

    HWND hWnd = CreateWindowExW(
        WS_EX_DLGMODALFRAME | WS_EX_TOPMOST,
        CLASS_NAME,
        L"Password Generator Settings",
        WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
        x, y, dlgW, dlgH,
        hParent, NULL, GetModuleHandle(NULL), &ctx
    );

    if (!hWnd) return false;

    // EnableWindow(hParent, FALSE) for modal behavior
    if (hParent) EnableWindow(hParent, FALSE);

    MSG msg;
    while (IsWindow(hWnd) && GetMessageW(&msg, NULL, 0, 0)) {
        if (!IsDialogMessageW(hWnd, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    if (hParent) {
        EnableWindow(hParent, TRUE);
        SetForegroundWindow(hParent);
    }

    UnregisterClassW(CLASS_NAME, GetModuleHandle(NULL));
    return ctx.result;
}

} // namespace win32
