cmake_minimum_required(VERSION 3.20)
project(Wols_CA_PasswordGenerator CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)

set(CORE_SOURCES
    src/core/SecureRandom.cpp
    src/core/PasswordGenerator.cpp
)

if (WIN32)
    set(PLATFORM_SOURCES
        src/platform/win32/Win32Tray.cpp
        src/platform/win32/Win32SettingsDialog.cpp
        src/platform/win32/Win32App.cpp
        src/platform/win32/resources.rc
    )
else()
    set(PLATFORM_SOURCES
        src/platform/linux/LinuxApp.cpp
    )
endif()

# Main Application Executable
if (WIN32)
    add_executable(Wols_CA_PasswordGenerator WIN32
        main.cpp
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
    )
    target_link_libraries(Wols_CA_PasswordGenerator PRIVATE
        bcrypt
        comctl32
        shell32
        user32
        gdi32
    )
else()
    add_executable(Wols_CA_PasswordGenerator
        main.cpp
        ${CORE_SOURCES}
        ${PLATFORM_SOURCES}
    )
endif()

# Unit Tests Executable
add_executable(test_generator
    tests/test_generator.cpp
    ${CORE_SOURCES}
)

if (WIN32)
    target_link_libraries(test_generator PRIVATE bcrypt)
endif()
