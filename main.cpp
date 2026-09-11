#include <iostream>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "platform/win32/Win32App.h"
#else
#include "platform/linux/LinuxApp.h"
#endif

#ifdef _WIN32

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow) {
    win32::Win32App app;
    return app.run(hInstance, nCmdShow);
}

int main(int argc, char* argv[]) {
    return wWinMain(GetModuleHandle(NULL), NULL, GetCommandLineW(), SW_SHOWDEFAULT);
}

#else

int main(int argc, char* argv[]) {
    linux_platform::LinuxApp app;
    return app.run(argc, argv);
}

#endif
