# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added
- **Automatische Google Drive & Bestaande Kluis Detectie bij Opstarten**:
  - Uitgebreide scan over alle virtuele stations (`D:` t/m `Z:` naar `My Drive`, `Mijn Drive`, `Google Drive`) en CloudStorage-locaties via `VaultStorage::getAvailableGoogleDriveDirectories()`.
  - Intelligente kluisdetectie (`VaultStorage::findExistingVault`): controleert bij opstarten of er al een bestaand en gevuld `vault.json` bestand aanwezig is (in Google Drive, OneDrive of lokaal) en stelt deze direct in als actieve kluislocatie.
  - Automatische synchronisatie van alle instellingen naar het gevonden cloud-bestand en het lokale OS-configuratiebestand (`config.json`).
- **Lokale OS Configuratie JSON & Cross-Platform Export (Windows / Linux / Android)**:
  - Altijd een lokaal `config.json` bestand in de user directory van het OS (`%USERPROFILE%/.wols_password_generator/config.json` / `~/.wols_password_generator/config.json`).
  - Cross-platform export en configuratiesjablonen gegenereerd voor **Windows**, **Linux** en **Android**, zodat configuraties direct gekopieerd of geëxporteerd kunnen worden.
  - Nieuw tabblad **Platform Configs** in `SettingsDialog` met realtime weergave van de JSON-configuratie, doelplatformselectie, klembordkopieerknop en bestands-exportmogelijkheid.
  - Bij het opstarten wordt eerst naar de lokale gebruikersdirectory gekeken voor de configuratie en kluis, vervolgens naar OneDrive (met accountkeuze bij meerdere accounts) en Google Drive.
- **Meervoudige OneDrive Accountdetectie & Selectie**:
  - Automatische detectie van alle aanwezige OneDrive-accounts en -mappen (`OneDrive`, `OneDrive - Personal`, `OneDrive - Zakelijk`, etc.).
  - Keuzemenu in `SettingsDialog` en opstartdialoogvenster om bij meerdere OneDrives direct de gewenste accountmap te selecteren.
- **Inline Bewerkbare Kaarten & Schone Weergave (`VaultDialog` & `AccountEditDialog`)**:
  - Weergavemodus toont uitsluitend ingevulde velden (gebruikersnaam, e-mailadres, wachtwoord, URL, notities) voor een rustig en overzichtelijk beeld.
  - Inline bewerkingsmodus voor zowel items als individuele accounts met alle invoervelden direct beschikbaar.
  - Icon-only actieknoppen voor Opslaan (`IconType::Save` disk-icoon) en Annuleren (`IconType::Cancel` kruis-icoon) met duidelijke tooltips.
  - Geïntegreerde wachtwoordgenerator en hold-to-reveal functie direct binnen de accountkaart in bewerkingsmodus.
- **Google Chrome CSV Wachtwoorden Importeren & Veilige Bestandswisser**:
  - Nieuwe `ChromeImporter` module voor het parsen en importeren van Google Chrome CSV-exportbestanden (conform RFC 4180).
  - Directe on-the-fly encryptie (`AES-256-GCM`) van geïmporteerde wachtwoorden en notities met de hoofdsleutel.
  - Automatische herkenning van kolommen (`name`, `url`, `username`, `password`, `note`), groepering in de kluis en intelligente samenvoeging van meerdere accounts per domein.
  - Veilige bestands-wisfunctie (`ChromeImporter::wipeFile`): na de import wordt de gebruiker gevraagd of het CSV-bronbestand leeggemaakt mag worden. Bij akkoord wordt het bestand overschreven met cryptografisch willekeurige bytes en nullen, en teruggebracht naar 0 bytes (bestand behouden maar volledig leeg).
  - Nieuwe werkbalkknop en sneltoets (`Ctrl+I`) in `VaultDialog` met speciaal vector-importicoon (`IconType::Import`).
  - Unit tests toegevoegd aan `test_vault` voor CSV-parsing, on-the-fly encryptie/decryptie en bestandsleegmaking.
- **UI / UX & Hotkeys (Stap 2)**:
  - Globale sneltoetsen toegevoegd aan `MainWindow` (F5 / Ctrl+G voor genereren, Ctrl+C voor kopiëren, Ctrl+S voor opslaan in kluis, Ctrl+K / Ctrl+O voor kluis openen, Ctrl+, / F2 voor instellingen).
  - Sneltoetsen toegevoegd aan `VaultDialog` (Ctrl+F voor zoeken, Ctrl+N voor nieuw item, Ctrl+Shift+N voor nieuwe groep, Del voor verwijderen, F2 voor hernoemen, Ctrl+S voor opslaan).
  - Volledig vernieuwd tabbed `SettingsDialog` met drie overzichtelijke categorieën: *Generator*, *Cloud & Opslag*, en *Beveiliging & Biometrie*.
  - Duidelijke interactieve tooltips en iconen met actieve focus- en statusaanduidingen.
- **Windows Hello & Biometrische Beveiliging (Stap 3)**:
  - Nieuwe `BiometricAuth` core module via Windows Credential UI / Hello API (`credui.lib`).
  - Configureerbare biometrische authenticatiepoorten voor:
    - Wachtwoord tonen (*Hold to Reveal*).
    - Wachtwoord kopiëren naar het klembord.
    - Openen van de wachtwoordkluis.
- **Cloud Synchronisatie & Multiplatform (Stap 4 - OneDrive & Google Drive focus)**:
  - Automatische detectie en padresolutie voor **Microsoft OneDrive** (`%OneDrive%`, `%OneDriveConsumer%`, `%OneDriveCommercial%`, Documents subfolder).
  - Automatische detectie voor **Google Drive Desktop** (virtuele stations `G:\My Drive` / `G:\Mijn Drive` en gebruikersprofielen).
  - Veilige kluismigratiefunctie (`VaultStorage::migrateVault`) met dialoogvenster om bestaande kluizen direct over te zetten bij het wijzigen van de opslaglocatie.
- **Kwaliteitsborging & Tests (Stap 1)**:
  - Unit test suite `test_vault` uitgebreid met automatische verificatie van cloudprovider-resolutie, kluismigratie, biometrische instellingen en JSON-serialisatie.
  - Release deployment pipeline geverifieerd via `rebuild_Release.ps1`.

### Fixed
- Weergave van corrupte knopteksten (`ðŸ”„`, `ðŸ”‹`, `âš™`) opgelost door `/utf-8` compiler-optie in te stellen in `CMakeLists.txt` en universele Unicode tekens te gebruiken voor de Generate-, Copy- en Settings-knoppen.
- Systeemvak- en dialoogiconen tonen nu direct het officiële applicatie-icoon in plaats van de standaard Windows placeholder.

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
