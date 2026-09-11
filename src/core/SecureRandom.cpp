#include "core/SecureRandom.h"
#include <random>
#include <stdexcept>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#pragma comment(lib, "bcrypt.lib")
#else
#include <sys/random.h>
#include <unistd.h>
#include <fcntl.h>
#endif

namespace core {

void SecureRandom::getBytes(void* buffer, size_t size) {
    if (size == 0) return;

#ifdef _WIN32
    NTSTATUS status = BCryptGenRandom(
        nullptr,
        reinterpret_cast<PUCHAR>(buffer),
        static_cast<ULONG>(size),
        BCRYPT_USE_SYSTEM_PREFERRED_RNG
    );
    if (status == 0) { // STATUS_SUCCESS
        return;
    }
#else
    ssize_t res = getrandom(buffer, size, 0);
    if (res == static_cast<ssize_t>(size)) {
        return;
    }
    // Fallback to /dev/urandom
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        size_t totalRead = 0;
        uint8_t* ptr = reinterpret_cast<uint8_t*>(buffer);
        while (totalRead < size) {
            ssize_t r = read(fd, ptr + totalRead, size - totalRead);
            if (r <= 0) break;
            totalRead += r;
        }
        close(fd);
        if (totalRead == size) return;
    }
#endif

    // Standard fallback using std::random_device
    std::random_device rd;
    uint8_t* bytePtr = reinterpret_cast<uint8_t*>(buffer);
    for (size_t i = 0; i < size; ++i) {
        bytePtr[i] = static_cast<uint8_t>(rd() & 0xFF);
    }
}

uint32_t SecureRandom::getUint32() {
    uint32_t val = 0;
    getBytes(&val, sizeof(val));
    return val;
}

int SecureRandom::getInt(int minInclusive, int maxInclusive) {
    if (minInclusive > maxInclusive) {
        std::swap(minInclusive, maxInclusive);
    }
    if (minInclusive == maxInclusive) {
        return minInclusive;
    }
    uint32_t range = static_cast<uint32_t>(maxInclusive - minInclusive + 1);
    
    // Lemire's nearly divisionless unbiased integer generation
    uint64_t random = getUint32();
    uint64_t multi = random * range;
    uint32_t leftover = static_cast<uint32_t>(multi);
    if (leftover < range) {
        uint32_t threshold = -range % range;
        while (leftover < threshold) {
            random = getUint32();
            multi = random * range;
            leftover = static_cast<uint32_t>(multi);
        }
    }
    return minInclusive + static_cast<int>(multi >> 32);
}

size_t SecureRandom::getIndex(size_t poolSize) {
    if (poolSize <= 1) return 0;
    return static_cast<size_t>(getInt(0, static_cast<int>(poolSize - 1)));
}

} // namespace core
