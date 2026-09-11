#include "core/PasswordGenerator.h"
#include "core/SecureRandom.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <vector>

namespace core {

namespace {
    const std::string LOWERCASE_CHARS = "abcdefghijklmnopqrstuvwxyz";
    const std::string UPPERCASE_CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    const std::string DIGIT_CHARS     = "0123456789";
    const std::string DEFAULT_SPECIAL = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
    const std::string HEX_UPPER       = "0123456789ABCDEF";
    const std::string HEX_LOWER       = "0123456789abcdef";

    bool isSpecialChar(char c, const std::string& specialSet) {
        return specialSet.find(c) != std::string::npos;
    }
}

std::string PasswordGenerator::generate(const PasswordOptions& rawOptions) {
    PasswordOptions opt = rawOptions;
    opt.sanitize();

    std::string specialPool = opt.specialCharSet.empty() ? DEFAULT_SPECIAL : opt.specialCharSet;
    std::string nonSpecialPool;
    std::string totalPool;

    if (opt.hexOnly) {
        nonSpecialPool = (opt.hexCase == HexCase::Uppercase) ? HEX_UPPER : HEX_LOWER;
        totalPool = nonSpecialPool;
    } else {
        std::string letterPool;
        if (opt.useLetters) {
            if (opt.letterCase == LetterCase::Both || opt.letterCase == LetterCase::LowerOnly) {
                letterPool += LOWERCASE_CHARS;
            }
            if (opt.letterCase == LetterCase::Both || opt.letterCase == LetterCase::UpperOnly) {
                letterPool += UPPERCASE_CHARS;
            }
        }
        
        std::string digitPool;
        if (opt.useDigits) {
            digitPool = DIGIT_CHARS;
        }

        nonSpecialPool = letterPool + digitPool;
        totalPool = nonSpecialPool;
        if (opt.useSpecialChars) {
            totalPool += specialPool;
        }

        if (totalPool.empty()) {
            nonSpecialPool = LOWERCASE_CHARS + UPPERCASE_CHARS + DIGIT_CHARS;
            totalPool = nonSpecialPool;
        } else if (nonSpecialPool.empty()) {
            nonSpecialPool = LOWERCASE_CHARS + UPPERCASE_CHARS + DIGIT_CHARS;
        }
    }

    std::string password;
    password.resize(opt.length);

    // Initial fill from total pool
    for (int i = 0; i < opt.length; ++i) {
        password[i] = totalPool[SecureRandom::getIndex(totalPool.size())];
    }

    // Apply Signature rule if enabled
    if (opt.useSignature) {
        int sigIdx = opt.signaturePosition - 1; // 0-based index
        if (sigIdx < 0) sigIdx = 0;
        if (sigIdx >= opt.length) sigIdx = opt.length - 1;

        // 1. Signature character MUST be a special character
        password[sigIdx] = specialPool[SecureRandom::getIndex(specialPool.size())];

        // 2. Character before signature (if exists) MUST NOT be a special character
        if (sigIdx - 1 >= 0) {
            password[sigIdx - 1] = nonSpecialPool[SecureRandom::getIndex(nonSpecialPool.size())];
        }

        // 3. Character after signature (if exists) MUST NOT be a special character
        if (sigIdx + 1 < opt.length) {
            password[sigIdx + 1] = nonSpecialPool[SecureRandom::getIndex(nonSpecialPool.size())];
        }
    }

    return password;
}

double PasswordGenerator::calculateEntropy(const PasswordOptions& rawOptions) {
    PasswordOptions opt = rawOptions;
    opt.sanitize();

    size_t poolSize = 0;
    if (opt.hexOnly) {
        poolSize = 16;
    } else {
        if (opt.useLetters) {
            if (opt.letterCase == LetterCase::Both) poolSize += 52;
            else poolSize += 26;
        }
        if (opt.useDigits) {
            poolSize += 10;
        }
        if (opt.useSpecialChars) {
            poolSize += opt.specialCharSet.empty() ? DEFAULT_SPECIAL.size() : opt.specialCharSet.size();
        }
    }

    if (poolSize == 0) poolSize = 62;
    return static_cast<double>(opt.length) * (std::log2(static_cast<double>(poolSize)));
}

std::string PasswordGenerator::getCharsetSummary(const PasswordOptions& rawOptions) {
    PasswordOptions opt = rawOptions;
    opt.sanitize();

    if (opt.hexOnly) {
        return (opt.hexCase == HexCase::Uppercase) ? "Hexadecimal (0-9, A-F)" : "Hexadecimal (0-9, a-f)";
    }

    std::vector<std::string> parts;
    if (opt.useLetters) {
        if (opt.letterCase == LetterCase::Both) parts.push_back("a-z, A-Z");
        else if (opt.letterCase == LetterCase::LowerOnly) parts.push_back("a-z");
        else if (opt.letterCase == LetterCase::UpperOnly) parts.push_back("A-Z");
    }
    if (opt.useDigits) {
        parts.push_back("0-9");
    }
    if (opt.useSpecialChars) {
        parts.push_back("Symbols");
    }
    if (opt.useSignature) {
        std::ostringstream ss;
        ss << "Signature @ pos " << opt.signaturePosition;
        parts.push_back(ss.str());
    }

    std::ostringstream res;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) res << " + ";
        res << parts[i];
    }
    return res.str();
}

} // namespace core
