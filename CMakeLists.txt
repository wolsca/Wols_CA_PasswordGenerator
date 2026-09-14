cmake_minimum_required(VERSION 3.20)
project(Wols_CA_PasswordGenerator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if (MSVC)
    add_compile_options(/utf-8)
endif()

# Fallback Qt6 discovery path if not already provided
if (NOT DEFINED CMAKE_PREFIX_PATH AND EXISTS "C:/Qt/6.11.1/msvc2022_64")
    set(CMAKE_PREFIX_PATH "C:/Qt/6.11.1/msvc2022_64")
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CORE_HEADERS
    include/core/PasswordOptions.h
    include/core/PasswordGenerator.h
    include/core/SecureRandom.h
    include/core/VaultModel.h
    include/core/VaultCrypto.h
    include/core/VaultStorage.h
    include/core/BiometricAuth.h
    include/core/ChromeImporter.h
)

set(CORE_SOURCES
    src/core/SecureRandom.cpp
    src/core/PasswordGenerator.cpp
    src/core/VaultModel.cpp
    src/core/VaultCrypto.cpp
    src/core/VaultStorage.cpp
    src/core/BiometricAuth.cpp
    src/core/ChromeImporter.cpp
)

set(GUI_HEADERS
    include/gui/IconUtils.h
    include/gui/MasterPasswordDialog.h
    include/gui/SettingsDialog.h
    include/gui/AccountEditDialog.h
    include/gui/VaultDialog.h
    include/gui/TrayManager.h
    include/gui/MainWindow.h
)

set(GUI_SOURCES
    src/gui/IconUtils.cpp
    src/gui/MasterPasswordDialog.cpp
    src/gui/SettingsDialog.cpp
    src/gui/AccountEditDialog.cpp
    src/gui/VaultDialog.cpp
    src/gui/TrayManager.cpp
    src/gui/MainWindow.cpp
)

# Main Executable
if (WIN32)
    add_executable(Wols_CA_PasswordGenerator WIN32
        main.cpp
        ${CORE_HEADERS}
        ${CORE_SOURCES}
        ${GUI_HEADERS}
        ${GUI_SOURCES}
    )
    target_link_libraries(Wols_CA_PasswordGenerator PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        bcrypt
        credui
    )
else()
    add_executable(Wols_CA_PasswordGenerator
        main.cpp
        ${CORE_HEADERS}
        ${CORE_SOURCES}
        ${GUI_HEADERS}
        ${GUI_SOURCES}
    )
    target_link_libraries(Wols_CA_PasswordGenerator PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
    )
endif()

# Generator Unit Tests Executable
add_executable(test_generator
    tests/test_generator.cpp
    src/core/SecureRandom.cpp
    src/core/PasswordGenerator.cpp
)
if (WIN32)
    target_link_libraries(test_generator PRIVATE bcrypt)
endif()

# Vault Unit Tests Executable
add_executable(test_vault
    tests/test_vault.cpp
    ${CORE_SOURCES}
)
target_link_libraries(test_vault PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
)
if (WIN32)
    target_link_libraries(test_vault PRIVATE bcrypt credui)
endif()
