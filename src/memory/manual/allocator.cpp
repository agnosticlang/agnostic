// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/manual/allocator.hpp"
#if defined(AGN_TARGET_FREEBSD)
#include "platform/freebsd/platform.hpp"
#elif defined(AGN_TARGET_WINDOWS)
#include "platform/windows/platform.hpp"
#elif defined(AGN_TARGET_HURD)
#include "platform/hurd/platform.hpp"
#else
#include "platform/linux/platform.hpp"
#endif

namespace agn::memory::manual {

namespace {

struct Header {
    uint64_t prevSize;
    uint64_t sizeAndFlags;
};

struct FreeLinks {
    Header* next;
    Header* prev;
};

constexpr uint64_t kInUse = 1;
constexpr uint64_t kPrevInUse = 2;
constexpr uint64_t kFlagMask = kInUse | kPrevInUse;
constexpr uint64_t kAlign = 16;
constexpr uint64_t kMinBlockSize = sizeof(Header) + sizeof(FreeLinks);
constexpr uint64_t kChunkSize = 1ULL << 16;
constexpr int kBinCount = 64;
constexpr uint64_t kMaxAllocSize = 1ULL << 47;

Header* bins[kBinCount];

[[noreturn]] void outOfMemory() {
    constexpr char message[] = "fatal error: out of memory\n";
    agn::platform::writeFd(2, message, sizeof(message) - 1);
    agn::platform::exitProcess(2);
}

uint64_t roundUp(uint64_t n, uint64_t align) {
    return (n + align - 1) & ~(align - 1);
}

uint64_t blockSize(const Header* b) { return b->sizeAndFlags & ~kFlagMask; }
bool inUse(const Header* b) { return (b->sizeAndFlags & kInUse) != 0; }
bool prevInUse(const Header* b) { return (b->sizeAndFlags & kPrevInUse) != 0; }
Header* nextBlock(Header* b) { return reinterpret_cast<Header*>(reinterpret_cast<char*>(b) + blockSize(b)); }
FreeLinks* links(Header* b) { return reinterpret_cast<FreeLinks*>(b + 1); }
int binIndex(uint64_t size) { return 63 - __builtin_clzll(size); }

void pushFree(Header* b) {
    Header*& head = bins[binIndex(blockSize(b))];
    links(b)->prev = nullptr;
    links(b)->next = head;
    if (head != nullptr) links(head)->prev = b;
    head = b;
}

void unlinkFree(Header* b) {
    FreeLinks* l = links(b);
    if (l->prev != nullptr) links(l->prev)->next = l->next;
    else bins[binIndex(blockSize(b))] = l->next;
    if (l->next != nullptr) links(l->next)->prev = l->prev;
}

void markFree(Header* b, uint64_t size) {
    b->sizeAndFlags = size | (b->sizeAndFlags & kPrevInUse);
    Header* next = nextBlock(b);
    next->prevSize = size;
    next->sizeAndFlags &= ~kPrevInUse;
    pushFree(b);
}

Header* mapChunk(uint64_t need) {
    uint64_t chunkSize = roundUp(need + sizeof(Header), kChunkSize);
    auto* chunk = static_cast<char*>(agn::platform::mapAnonymous(chunkSize));
    if (chunk == nullptr) outOfMemory();
    auto* fence = reinterpret_cast<Header*>(chunk + chunkSize - sizeof(Header));
    fence->sizeAndFlags = kInUse;
    auto* block = reinterpret_cast<Header*>(chunk);
    block->sizeAndFlags = kPrevInUse;
    markFree(block, chunkSize - sizeof(Header));
    return block;
}

Header* findFree(uint64_t need) {
    int first = binIndex(need);
    Header* best = nullptr;
    for (Header* b = bins[first]; b != nullptr; b = links(b)->next) {
        uint64_t size = blockSize(b);
        if (size < need || (best != nullptr && size >= blockSize(best))) continue;
        best = b;
        if (size == need) break;
    }
    if (best != nullptr) return best;
    for (int i = first + 1; i < kBinCount; i++) {
        if (bins[i] != nullptr) return bins[i];
    }
    return nullptr;
}

} // namespace

void* alloc(uint64_t size) {
    if (size > kMaxAllocSize) outOfMemory();
    uint64_t need = roundUp(size + sizeof(Header), kAlign);
    if (need < kMinBlockSize) need = kMinBlockSize;

    Header* b = findFree(need);
    if (b == nullptr) b = mapChunk(need);
    unlinkFree(b);

    uint64_t available = blockSize(b);
    if (available - need >= kMinBlockSize) {
        b->sizeAndFlags = need | kInUse | (b->sizeAndFlags & kPrevInUse);
        Header* rest = nextBlock(b);
        rest->sizeAndFlags = kPrevInUse;
        markFree(rest, available - need);
    } else {
        b->sizeAndFlags |= kInUse;
        nextBlock(b)->sizeAndFlags |= kPrevInUse;
    }
    return b + 1;
}

void free(void* ptr) {
    if (ptr == nullptr) return;
    Header* b = static_cast<Header*>(ptr) - 1;
    uint64_t size = blockSize(b);

    Header* next = nextBlock(b);
    if (!inUse(next)) {
        unlinkFree(next);
        size += blockSize(next);
    }
    if (!prevInUse(b)) {
        auto* prev = reinterpret_cast<Header*>(reinterpret_cast<char*>(b) - b->prevSize);
        unlinkFree(prev);
        size += blockSize(prev);
        b = prev;
    }
    markFree(b, size);
}

} // namespace agn::memory::manual
