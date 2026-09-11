#pragma once
#include "core/PasswordOptions.h"
#include <string>

namespace ui {

class AppConfig {
public:
    static core::PasswordOptions& getOptions() {
        static core::PasswordOptions options;
        return options;
    }

    static void loadDefaults() {
        core::PasswordOptions& opt = getOptions();
        opt.length = 16;
        opt.useLetters = true;
        opt.letterCase = core::LetterCase::Both;
        opt.useDigits = true;
        opt.useSpecialChars = true;
        opt.specialCharSet = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
        opt.hexOnly = false;
        opt.hexCase = core::HexCase::Uppercase;
        opt.useSignature = false;
        opt.signaturePosition = 2;
    }
};

} // namespace ui
