#pragma once

#include <QString>
#include <QWidget>

namespace core {

class BiometricAuth {
public:
    // Check if biometric/OS credential authentication is available
    static bool isAvailable();

    // Prompts the user with Windows Hello / OS biometric / PIN verification
    static bool authenticate(const QString& reason, QWidget* parent = nullptr);
};

} // namespace core
