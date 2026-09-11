#pragma once
#include <string>

namespace core {

enum class LetterCase {
    Both,       // Uppercase & Lowercase (a-z, A-Z)
    LowerOnly,  // Lowercase only (a-z)
    UpperOnly   // Uppercase only (A-Z)
};

enum class HexCase {
    Uppercase,  // 0-9, A-F
    Lowercase   // 0-9, a-f
};

struct PasswordOptions {
    int length = 16;                     // Allowed range: 8 to 128
    
    // Character set options
    bool useLetters = true;
    LetterCase letterCase = LetterCase::Both;
    bool useDigits = true;
    bool useSpecialChars = true;
    std::string specialCharSet = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
    
    // Hexadecimal only option
    bool hexOnly = false;
    HexCase hexCase = HexCase::Uppercase;

    // Signature option:
    // When enabled, the character at signaturePosition (1-based index, default: 2)
    // is guaranteed to be a special character, and the adjacent characters (1st and 3rd for pos 2)
    // are guaranteed NOT to be special characters (they come from non-special enabled characters).
    bool useSignature = false;
    int signaturePosition = 2; // 1-based index (e.g., 2 means the 2nd character)

    // Helper validation
    void sanitize() {
        if (length < 8) length = 8;
        if (length > 128) length = 128;
        if (signaturePosition < 1) signaturePosition = 1;
        if (signaturePosition > length) signaturePosition = length;
        if (specialCharSet.empty()) {
            specialCharSet = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
        }
        if (!hexOnly && !useLetters && !useDigits && !useSpecialChars) {
            // Fallback if user deselects everything
            useLetters = true;
            useDigits = true;
        }
    }
};

} // namespace core
