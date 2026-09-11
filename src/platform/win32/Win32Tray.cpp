#include "Win32Tray.h"
#include "resource.h"

namespace win32 {

Win32Tray::Win32Tray() {
    ZeroMemory(&m_nid, sizeof(m_nid));
    m_nid.cbSize = sizeof(NOTIFYICONDATAW);
}

Win32Tray::~Win32Tray() {
    remove();
}

bool Win32Tray::init(HWND hWnd, UINT callbackMsg, HICON hIcon, const std::wstring& tip) {
    if (m_visible) {
        remove();
    }
    m_nid.hWnd = hWnd;
    m_nid.uID = 1;
    m_nid.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
    m_nid.uCallbackMessage = callbackMsg;
    m_nid.hIcon = hIcon ? hIcon : LoadIcon(NULL, IDI_APPLICATION);
    wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);

    if (Shell_NotifyIconW(NIM_ADD, &m_nid)) {
        m_nid.uVersion = NOTIFYICON_VERSION_4;
        Shell_NotifyIconW(NIM_SETVERSION, &m_nid);
        m_visible = true;
        return true;
    }
    return false;
}

void Win32Tray::updateTooltip(const std::wstring& tip) {
    if (!m_visible) return;
    wcsncpy_s(m_nid.szTip, tip.c_str(), _TRUNCATE);
    m_nid.uFlags = NIF_TIP;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void Win32Tray::showBalloon(const std::wstring& title, const std::wstring& message, DWORD infoFlags) {
    if (!m_visible) return;
    m_nid.uFlags = NIF_INFO;
    wcsncpy_s(m_nid.szInfoTitle, title.c_str(), _TRUNCATE);
    wcsncpy_s(m_nid.szInfo, message.c_str(), _TRUNCATE);
    m_nid.dwInfoFlags = infoFlags;
    Shell_NotifyIconW(NIM_MODIFY, &m_nid);
}

void Win32Tray::remove() {
    if (m_visible) {
        Shell_NotifyIconW(NIM_DELETE, &m_nid);
        m_visible = false;
    }
}

void Win32Tray::showContextMenu(HWND hWnd) {
    POINT pt;
    GetCursorPos(&pt);
    HMENU hMenu = CreatePopupMenu();
    if (!hMenu) return;

    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_SHOW, L"&Show Password Generator");
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_GENERATE_COPY, L"&Generate && Copy Password");
    AppendMenuW(hMenu, MF_SEPARATOR, 0, nullptr);
    AppendMenuW(hMenu, MF_STRING, IDM_TRAY_EXIT, L"E&xit");

    // Set default menu item to Show (bold)
    SetMenuDefaultItem(hMenu, IDM_TRAY_SHOW, FALSE);

    // Required workaround for Win32 tray menus
    SetForegroundWindow(hWnd);
    TrackPopupMenu(hMenu, TPM_RIGHTBUTTON | TPM_NOANIMATION, pt.x, pt.y, 0, hWnd, NULL);
    PostMessageW(hWnd, WM_NULL, 0, 0);

    DestroyMenu(hMenu);
}

} // namespace win32
