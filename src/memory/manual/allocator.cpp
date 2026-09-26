// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/manual/allocator.hpp"
#if defined(AGN_TARGET_FREEBSD)
#include "platform/freebsd/platform.hpp"
#else
#include "platform/linux/platform.hpp"
#endif

namespace agn::memory::manual {

namespace {

struct Block {
    uint64_t size;
    Block* next;
};

constexpr uint64_t kAlign = 16;
constexpr uint64_t kChunkSize = 1UL << 16;

Block* freeList = nullptr;

uint64_t roundUp(uint64_t n, uint64_t align) {
    return (n + align - 1) & ~(align - 1);
}

} // namespace

void* alloc(uint64_t size) {
    size = roundUp(size, kAlign);

    Block** prev = &freeList;
    for (Block* b = freeList; b != nullptr; b = b->next) {
        if (b->size >= size) {
            *prev = b->next;
            return reinterpret_cast<char*>(b) + sizeof(Block);
        }
        prev = &b->next;
    }

    uint64_t mapSize = roundUp(size + sizeof(Block), kChunkSize);
    void* mem = agn::platform::mapAnonymous(mapSize);
    if (mem == nullptr) return nullptr;

    Block* b = reinterpret_cast<Block*>(mem);
    b->size = mapSize - sizeof(Block);
    b->next = nullptr;
    return reinterpret_cast<char*>(b) + sizeof(Block);
}

void free(void* ptr) {
    if (ptr == nullptr) return;
    Block* b = reinterpret_cast<Block*>(reinterpret_cast<char*>(ptr) - sizeof(Block));
    b->next = freeList;
    freeList = b;
}

} // namespace agn::memory::manual
