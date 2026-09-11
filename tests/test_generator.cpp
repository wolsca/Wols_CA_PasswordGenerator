#include "core/PasswordGenerator.h"
#include <cassert>
#include <iostream>
#include <set>
#include <string>

void testLengths() {
    std::cout << "[TEST] Testing password lengths (8 to 128)..." << std::endl;
    for (int len : {8, 12, 16, 24, 32, 64, 128}) {
        core::PasswordOptions opt;
        opt.length = len;
        std::string pwd = core::PasswordGenerator::generate(opt);
        assert(static_cast<int>(pwd.length()) == len);
    }
    // Test clamping
    {
        core::PasswordOptions opt;
        opt.length = 1;
        std::string pwd = core::PasswordGenerator::generate(opt);
        assert(pwd.length() == 8);
    }
    {
        core::PasswordOptions opt;
        opt.length = 300;
        std::string pwd = core::PasswordGenerator::generate(opt);
        assert(pwd.length() == 128);
    }
    std::cout << "  -> Lengths passed!" << std::endl;
}

void testLetterCases() {
    std::cout << "[TEST] Testing letter cases..." << std::endl;
    // Lowercase only
    {
        core::PasswordOptions opt;
        opt.length = 50;
        opt.useLetters = true;
        opt.letterCase = core::LetterCase::LowerOnly;
        opt.useDigits = false;
        opt.useSpecialChars = false;
        opt.useSignature = false;
        std::string pwd = core::PasswordGenerator::generate(opt);
        for (char c : pwd) {
            assert(c >= 'a' && c <= 'z');
        }
    }
    // Uppercase only
    {
        core::PasswordOptions opt;
        opt.length = 50;
        opt.useLetters = true;
        opt.letterCase = core::LetterCase::UpperOnly;
        opt.useDigits = false;
        opt.useSpecialChars = false;
        opt.useSignature = false;
        std::string pwd = core::PasswordGenerator::generate(opt);
        for (char c : pwd) {
            assert(c >= 'A' && c <= 'Z');
        }
    }
    // Both
    {
        core::PasswordOptions opt;
        opt.length = 100;
        opt.useLetters = true;
        opt.letterCase = core::LetterCase::Both;
        opt.useDigits = false;
        opt.useSpecialChars = false;
        opt.useSignature = false;
        std::string pwd = core::PasswordGenerator::generate(opt);
        bool hasLower = false, hasUpper = false;
        for (char c : pwd) {
            if (c >= 'a' && c <= 'z') hasLower = true;
            if (c >= 'A' && c <= 'Z') hasUpper = true;
        }
        assert(hasLower && hasUpper);
    }
    std::cout << "  -> Letter cases passed!" << std::endl;
}

void testHexadecimal() {
    std::cout << "[TEST] Testing hexadecimal mode..." << std::endl;
    // Uppercase Hex
    {
        core::PasswordOptions opt;
        opt.length = 64;
        opt.hexOnly = true;
        opt.hexCase = core::HexCase::Uppercase;
        opt.useSignature = false;
        std::string pwd = core::PasswordGenerator::generate(opt);
        for (char c : pwd) {
            bool isHex = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F');
            assert(isHex);
        }
    }
    // Lowercase Hex
    {
        core::PasswordOptions opt;
        opt.length = 64;
        opt.hexOnly = true;
        opt.hexCase = core::HexCase::Lowercase;
        opt.useSignature = false;
        std::string pwd = core::PasswordGenerator::generate(opt);
        for (char c : pwd) {
            bool isHex = (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
            assert(isHex);
        }
    }
    std::cout << "  -> Hexadecimal mode passed!" << std::endl;
}

void testSignatureRule() {
    std::cout << "[TEST] Testing signature rule..." << std::endl;
    std::string specialSet = "!@#$%^&*()_+-=[]{}|;:,.<>?/~";
    auto isSpecial = [&](char c) {
        return specialSet.find(c) != std::string::npos;
    };

    // 1. Position 2 (Default: 2nd character is special, 1st and 3rd are not special)
    for (int iter = 0; iter < 100; ++iter) {
        core::PasswordOptions opt;
        opt.length = 16;
        opt.useLetters = true;
        opt.useDigits = true;
        opt.useSpecialChars = true;
        opt.useSignature = true;
        opt.signaturePosition = 2; // 2nd character
        std::string pwd = core::PasswordGenerator::generate(opt);
        assert(pwd.length() == 16);

        // Character 1 (index 0) must NOT be special
        assert(!isSpecial(pwd[0]));
        // Character 2 (index 1) MUST be special
        assert(isSpecial(pwd[1]));
        // Character 3 (index 2) must NOT be special
        assert(!isSpecial(pwd[2]));
    }

    // 2. Position 1 (1st character is special, 2nd is not special)
    for (int iter = 0; iter < 50; ++iter) {
        core::PasswordOptions opt;
        opt.length = 16;
        opt.useLetters = true;
        opt.useDigits = true;
        opt.useSpecialChars = true;
        opt.useSignature = true;
        opt.signaturePosition = 1; // 1st character (index 0)
        std::string pwd = core::PasswordGenerator::generate(opt);

        // Character 1 (index 0) MUST be special
        assert(isSpecial(pwd[0]));
        // Character 2 (index 1) must NOT be special
        assert(!isSpecial(pwd[1]));
    }

    // 3. Position N (last character: e.g. pos 16 for length 16)
    for (int iter = 0; iter < 50; ++iter) {
        core::PasswordOptions opt;
        opt.length = 16;
        opt.useLetters = true;
        opt.useDigits = true;
        opt.useSpecialChars = true;
        opt.useSignature = true;
        opt.signaturePosition = 16;
        std::string pwd = core::PasswordGenerator::generate(opt);

        // Character 15 (index 14) must NOT be special
        assert(!isSpecial(pwd[14]));
        // Character 16 (index 15) MUST be special
        assert(isSpecial(pwd[15]));
    }

    // 4. Custom position (e.g. pos 7)
    for (int iter = 0; iter < 50; ++iter) {
        core::PasswordOptions opt;
        opt.length = 24;
        opt.useLetters = true;
        opt.useDigits = true;
        opt.useSpecialChars = true;
        opt.useSignature = true;
        opt.signaturePosition = 7;
        std::string pwd = core::PasswordGenerator::generate(opt);

        assert(!isSpecial(pwd[5])); // pos 6 (index 5)
        assert(isSpecial(pwd[6]));  // pos 7 (index 6)
        assert(!isSpecial(pwd[7])); // pos 8 (index 7)
    }

    std::cout << "  -> Signature rule passed!" << std::endl;
}

void testCustomSpecialCharacters() {
    std::cout << "[TEST] Testing custom special characters..." << std::endl;
    core::PasswordOptions opt;
    opt.length = 32;
    opt.useLetters = false;
    opt.useDigits = false;
    opt.useSpecialChars = true;
    opt.specialCharSet = "@#";
    opt.useSignature = false;
    std::string pwd = core::PasswordGenerator::generate(opt);
    for (char c : pwd) {
        assert(c == '@' || c == '#');
    }
    std::cout << "  -> Custom special characters passed!" << std::endl;
}

int main() {
    std::cout << "=== Running Password Generator Comprehensive Tests ===" << std::endl;
    testLengths();
    testLetterCases();
    testHexadecimal();
    testSignatureRule();
    testCustomSpecialCharacters();
    std::cout << "=== All Comprehensive Tests Passed Successfully! ===" << std::endl;
    return 0;
}
