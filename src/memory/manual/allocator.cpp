// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/manual/allocator.hpp"
#include "platform/linux/platform.hpp"

namespace agn::memory::manual {

namespace {

struct Block {
    unsigned long size;
    Block* next;
};

constexpr unsigned long kAlign = 16;
constexpr unsigned long kChunkSize = 1UL << 16;

Block* freeList = nullptr;

unsigned long roundUp(unsigned long n, unsigned long align) {
    return (n + align - 1) & ~(align - 1);
}

} // namespace

void* alloc(unsigned long size) {
    size = roundUp(size, kAlign);

    Block** prev = &freeList;
    for (Block* b = freeList; b != nullptr; b = b->next) {
        if (b->size >= size) {
            *prev = b->next;
            return reinterpret_cast<char*>(b) + sizeof(Block);
        }
        prev = &b->next;
    }

    unsigned long mapSize = roundUp(size + sizeof(Block), kChunkSize);
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
