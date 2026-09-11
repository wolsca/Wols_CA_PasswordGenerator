#pragma once
#include "core/PasswordOptions.h"

namespace linux_platform {

class LinuxApp {
public:
    LinuxApp();
    ~LinuxApp();

    int run(int argc, char* argv[]);

private:
    core::PasswordOptions m_options;
    void runInteractiveUI();
    void showHelp();
};

} // namespace linux_platform
