// A memory pool reduce the overload caused by frequent calls to malloc/free
// ensure the allocated memory is contiguous, enhancing cache locality
// 减少频繁调用 malloc/free 造成的开销，并确保分配的内存是连续的，增强缓存局部性
#include "arena.h"

namespace kirisamedb {

    Arena::Arena() : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0) {}
    Arena::~Arena() {
        for (size_t i = 0; i < blocks_.size(); i++) {
        delete[] blocks_[i];
        }
    }

    static const int kBlockSize = 4096;

    char* Arena::AllocateFallback(size_t bytes) {// 内存不足时new block的inpl
        if (bytes > kBlockSize / 4) {
        // leveldb基于跳表设计data structure，内存的分配非常频繁，大块内存直接new block减少碎片化
        char* result = AllocateNewBlock(bytes);
        return result;
        }

        // 需要调用AllocateFallback时，之前的块内存不再使用
        alloc_ptr_ = AllocateNewBlock(kBlockSize);
        alloc_bytes_remaining_ = kBlockSize;

        char* result = alloc_ptr_;
        alloc_ptr_ += bytes;
        alloc_bytes_remaining_ -= bytes;
        return result;
    }

    char* Arena::AllocateAligned(size_t bytes) {
        const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;
        static_assert((align & (align - 1)) == 0, "Pointer size should be a power of 2");
        size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
        size_t slop = (current_mod == 0 ? 0 : align - current_mod);
        size_t needed = bytes + slop;
        char* result;
        if (needed <= alloc_bytes_remaining_) {
            result = alloc_ptr_ + slop;
            alloc_ptr_ += needed;
            alloc_bytes_remaining_ -= needed;
    } else {
        // AllocateFallback always returned aligned memory
        result = AllocateFallback(bytes);
    }
    assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
        return result;
    }

    char* Arena::AllocateNewBlock(size_t block_bytes) {
        char* result = new char[block_bytes];
        blocks_.push_back(result);
        memory_usage_.fetch_add(block_bytes + sizeof(char*), std::memory_order_relaxed);
        return result;
    }
}  // namespace kirisamedb
