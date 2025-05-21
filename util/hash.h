// Simple hash function used for internal data structures

#ifndef STORAGE_UTIL_HASH_H_
#define STORAGE_UTIL_HASH_H_

#include <cstddef>
#include <cstdint>

namespace kirisamedb {
    uint32_t Hash(const char* data, size_t n, uint32_t seed);
}  // namespace kirisamedb

#endif  // STORAGE_HASH_H_