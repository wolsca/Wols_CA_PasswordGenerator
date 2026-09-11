#pragma once
#include <windows.h>
#include <shellapi.h>
#include <string>

namespace win32 {

class Win32Tray {
public:
    Win32Tray();
    ~Win32Tray();

    bool init(HWND hWnd, UINT callbackMsg, HICON hIcon, const std::wstring& tip);
    void updateTooltip(const std::wstring& tip);
    void showBalloon(const std::wstring& title, const std::wstring& message, DWORD infoFlags = NIIF_INFO);
    void remove();
    bool isVisible() const { return m_visible; }
    void showContextMenu(HWND hWnd);

private:
    NOTIFYICONDATAW m_nid;
    bool m_visible = false;
};

} // namespace win32
