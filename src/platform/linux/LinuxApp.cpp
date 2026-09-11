#include "LinuxApp.h"
#include "core/PasswordGenerator.h"
#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>

namespace linux_platform {

LinuxApp::LinuxApp() {
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

LinuxApp::~LinuxApp() {
}

void LinuxApp::showHelp() {
    std::cout << "Password Generator - Cross Platform (Linux / Windows)\n";
    std::cout << "Usage:\n";
    std::cout << "  --length <8-128>        Set password length (default: 16)\n";
    std::cout << "  --no-letters            Disable letters\n";
    std::cout << "  --lower-only            Use only lowercase letters\n";
    std::cout << "  --upper-only            Use only uppercase letters\n";
    std::cout << "  --no-digits             Disable digits\n";
    std::cout << "  --no-special            Disable special characters\n";
    std::cout << "  --hex                   Hexadecimal mode only\n";
    std::cout << "  --hex-lower             Hexadecimal lowercase mode\n";
    std::cout << "  --signature [pos]       Enable signature (default pos: 2)\n";
    std::cout << "  --copy                  Copy generated password to X11 clipboard (xclip/wl-copy)\n";
    std::cout << "  --help                  Show this help\n";
}

int LinuxApp::run(int argc, char* argv[]) {
    bool copyToClip = false;
    bool interactive = (argc <= 1);

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            showHelp();
            return 0;
        } else if (arg == "--length" && i + 1 < argc) {
            m_options.length = std::stoi(argv[++i]);
        } else if (arg == "--no-letters") {
            m_options.useLetters = false;
        } else if (arg == "--lower-only") {
            m_options.letterCase = core::LetterCase::LowerOnly;
        } else if (arg == "--upper-only") {
            m_options.letterCase = core::LetterCase::UpperOnly;
        } else if (arg == "--no-digits") {
            m_options.useDigits = false;
        } else if (arg == "--no-special") {
            m_options.useSpecialChars = false;
        } else if (arg == "--hex") {
            m_options.hexOnly = true;
        } else if (arg == "--hex-lower") {
            m_options.hexOnly = true;
            m_options.hexCase = core::HexCase::Lowercase;
        } else if (arg == "--signature") {
            m_options.useSignature = true;
            if (i + 1 < argc && argv[i+1][0] != '-') {
                m_options.signaturePosition = std::stoi(argv[++i]);
            }
        } else if (arg == "--copy") {
            copyToClip = true;
        }
    }

    if (interactive) {
        runInteractiveUI();
        return 0;
    }

    m_options.sanitize();
    std::string pwd = core::PasswordGenerator::generate(m_options);
    std::cout << pwd << std::endl;

    if (copyToClip) {
        // Try xclip or wl-copy if available
        std::string cmd = "printf '%s' '" + pwd + "' | (wl-copy 2>/dev/null || xclip -selection clipboard 2>/dev/null || xsel --clipboard --input 2>/dev/null)";
        (void)std::system(cmd.c_str());
    }

    return 0;
}

void LinuxApp::runInteractiveUI() {
    std::cout << "====================================================\n";
    std::cout << "             PASSWORD GENERATOR (Linux)             \n";
    std::cout << "====================================================\n\n";

    while (true) {
        m_options.sanitize();
        std::string pwd = core::PasswordGenerator::generate(m_options);
        double entropy = core::PasswordGenerator::calculateEntropy(m_options);
        std::string summary = core::PasswordGenerator::getCharsetSummary(m_options);

        std::cout << "\n----------------------------------------------------\n";
        std::cout << "Generated Password: \n\033[1;32m" << pwd << "\033[0m\n";
        std::cout << "----------------------------------------------------\n";
        std::cout << "Length: " << m_options.length << " | Entropy: ~" << (int)entropy << " bits | Options: " << summary << "\n\n";
        std::cout << "[R] 🔄 Generate / Refresh\n";
        std::cout << "[C] 📋 Copy to Clipboard\n";
        std::cout << "[L] Set Length (8-128)\n";
        std::cout << "[S] ⚙ Settings (Character Sets & Signature)\n";
        std::cout << "[Q] Quit\n";
        std::cout << "Choose an option: ";

        std::string choice;
        if (!(std::cin >> choice)) break;

        if (choice == "r" || choice == "R") {
            continue;
        } else if (choice == "c" || choice == "C") {
            std::string cmd = "printf '%s' '" + pwd + "' | (wl-copy 2>/dev/null || xclip -selection clipboard 2>/dev/null || xsel --clipboard --input 2>/dev/null)";
            (void)std::system(cmd.c_str());
            std::cout << "\n✓ Password copied to clipboard!\n";
        } else if (choice == "l" || choice == "L") {
            std::cout << "Enter new length (8-128): ";
            int newLen = 16;
            if (std::cin >> newLen) {
                m_options.length = newLen;
            }
        } else if (choice == "s" || choice == "S") {
            std::cout << "\n--- Settings ---\n";
            std::cout << "1. Toggle Hexadecimal Only (Current: " << (m_options.hexOnly ? "ON" : "OFF") << ")\n";
            std::cout << "2. Toggle Letters (Current: " << (m_options.useLetters ? "ON" : "OFF") << ")\n";
            std::cout << "3. Letter Case (Current: " << (m_options.letterCase == core::LetterCase::Both ? "Both" : (m_options.letterCase == core::LetterCase::LowerOnly ? "Lower only" : "Upper only")) << ")\n";
            std::cout << "4. Toggle Digits (Current: " << (m_options.useDigits ? "ON" : "OFF") << ")\n";
            std::cout << "5. Toggle Special Characters (Current: " << (m_options.useSpecialChars ? "ON" : "OFF") << ")\n";
            std::cout << "6. Toggle Signature (Current: " << (m_options.useSignature ? "ON" : "OFF") << ", Pos: " << m_options.signaturePosition << ")\n";
            std::cout << "7. Change Signature Position\n";
            std::cout << "Enter option (1-7) or 0 to back: ";
            int sChoice;
            if (std::cin >> sChoice) {
                if (sChoice == 1) m_options.hexOnly = !m_options.hexOnly;
                else if (sChoice == 2) m_options.useLetters = !m_options.useLetters;
                else if (sChoice == 3) {
                    int c;
                    std::cout << "1. Both, 2. Lower only, 3. Upper only: ";
                    if (std::cin >> c) {
                        if (c == 1) m_options.letterCase = core::LetterCase::Both;
                        else if (c == 2) m_options.letterCase = core::LetterCase::LowerOnly;
                        else if (c == 3) m_options.letterCase = core::LetterCase::UpperOnly;
                    }
                }
                else if (sChoice == 4) m_options.useDigits = !m_options.useDigits;
                else if (sChoice == 5) m_options.useSpecialChars = !m_options.useSpecialChars;
                else if (sChoice == 6) m_options.useSignature = !m_options.useSignature;
                else if (sChoice == 7) {
                    std::cout << "Enter signature position (1-" << m_options.length << "): ";
                    int p;
                    if (std::cin >> p) m_options.signaturePosition = p;
                }
            }
        } else if (choice == "q" || choice == "Q") {
            break;
        }
    }
}

} // namespace linux_platform
