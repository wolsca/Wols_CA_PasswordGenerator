#pragma once
#include <cstddef>
#include <cstdint>
#include <vector>

namespace core {

class SecureRandom {
public:
    static void getBytes(void* buffer, size_t size);
    static int getInt(int minInclusive, int maxInclusive);
    static uint32_t getUint32();
    static size_t getIndex(size_t poolSize);
};

} // namespace core
