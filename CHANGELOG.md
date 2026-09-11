# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- Added `commit.ps1` PowerShell script to automate Git commits and pushes using the description from `CHANGELOG.md`.
- Added `CHANGELOG.md` tracking all version histories, features, and changes since the latest commit.

### Changed
- Updated `README.md` with documentation and usage instructions for `commit.ps1` and `CHANGELOG.md`.

---

## [1.0.0] - 2026-09-11

### Added
- Initial cross-platform C++20 password generator core engine with cryptographically secure random generator (`BCryptGenRandom` on Windows, `/dev/urandom` on Linux).
- Configurable character set options (letters with uppercase/lowercase/both options, digits, custom special characters, hex-only mode).
- Signature feature for ownership recognition (customizable special character position with non-special adjacent characters).
- Direct length adjustment slider and spinner (8 to 128 characters).
- Native Win32 desktop GUI with system tray integration, balloon notifications, copy button, and settings dialog.
- Linux platform layer with interactive CLI, command-line arguments, and X11/Wayland clipboard support.
- CMake and Ninja build configuration with unit test suite (`test_generator`).
- PowerShell automation scripts: `rebuild_Release.ps1`, `start_release.ps1`, `start_debug.ps1`.
- GitHub repository integration (`wolsca/Wols_CA_PasswordGenerator`).
