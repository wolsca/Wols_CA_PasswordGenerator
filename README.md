# Wols Password Generator (Windows & Linux)

A fast, secure, and cross-platform C++20 password generator application with system tray integration and signature verification capabilities.

## Features

- **Character Set Options**:
  - **Letters**: Choose between both uppercase & lowercase (`a-z`, `A-Z`), lowercase only (`a-z`), or uppercase only (`A-Z`).
  - **Digits**: Include numbers (`0-9`).
  - **Special Characters**: Include standard symbols (`!@#$%^&*()_+-=[]{}|;:,.<>?/~`) or customize the symbol pool.
  - **Hexadecimal Only**: Option to generate purely hexadecimal passwords in uppercase (`0-9, A-F`) or lowercase (`0-9, a-f`).

- **Length Adjustment (8 to 128 characters)**:
  - Accessible directly on the main screen.
  - Interactive slider trackbar and synchronized numeric input/spinner.

- **Signature Feature (Ownership Mark)**:
  - Allows you to recognize passwords created by you.
  - Guarantees a special character at a specific position (default: 2nd character), while ensuring adjacent characters (1st and 3rd characters for position 2) are non-special.
  - Configurable position from 1 up to the total password length.

- **Clean & Intuitive User Interface (in English)**:
  - **Main Window**: Password display box, direct Length slider, `🔄 Generate` (refresh) button, `📋 Copy` button (with instant feedback), and `⚙ Settings` button.
  - **Settings Dialog**: Easily configure character sets, hexadecimal mode, and signature options.

- **System Tray Integration**:
  - When minimized, the application docks into the System Tray / Notification area.
  - Left-click or double-click on the tray icon restores the window.
  - Right-click tray context menu offers:
    - **Show Password Generator**
    - **Generate & Copy Password** (instantly generates in the background, copies to clipboard, and shows balloon notification)
    - **Exit**

- **Cross-Platform C++20**:
  - **Windows**: Native Win32 API with modern visual styles (Comctl32 v6 manifest), Shell_NotifyIcon, and Windows clipboard API.
  - **Linux**: Interactive CLI/GUI runner with X11/Wayland clipboard support (`wl-copy`, `xclip`, `xsel`).

---

## Building and Running

### Requirements
- C++20 compatible compiler (MSVC 2022/2026, GCC 11+, or Clang 13+)
- CMake 3.20+
- Ninja or Make

### Windows (PowerShell Scripts)

Convenience scripts are provided for building and launching:
- **Rebuild Release**: `.\rebuild_Release.ps1`
- **Start Release**: `.\start_release.ps1`
- **Start Debug**: `.\start_debug.ps1`

### Manual Windows Build
```powershell
cmake -B cmake-build-debug -G Ninja
cmake --build cmake-build-debug
.\cmake-build-debug\Wols_CA_PasswordGenerator.exe
```

### Linux
```bash
cmake -B build
cmake --build build
./build/Wols_CA_PasswordGenerator
```

### Running Tests
```bash
cmake --build cmake-build-debug --target test_generator
./cmake-build-debug/test_generator.exe # on Windows
./build/test_generator                # on Linux
```
