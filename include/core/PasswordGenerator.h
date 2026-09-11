#pragma once
#include "PasswordOptions.h"
#include <string>

namespace core {

class PasswordGenerator {
public:
    static std::string generate(const PasswordOptions& options);
    
    // Helper to calculate estimated entropy in bits
    static double calculateEntropy(const PasswordOptions& options);

    // Get character pool description
    static std::string getCharsetSummary(const PasswordOptions& options);
};

} // namespace core
