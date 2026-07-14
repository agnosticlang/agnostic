// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/arc/arc.hpp"
#include "memory/manual/allocator.hpp"

namespace agn::memory::arc {

namespace {
struct Header { unsigned long refcount; };
}

void* alloc(unsigned long size) {
    void* raw = manual::alloc(size + sizeof(Header));
    if (raw == nullptr) return nullptr;
    auto* h = reinterpret_cast<Header*>(raw);
    h->refcount = 1;
    return reinterpret_cast<char*>(raw) + sizeof(Header);
}

void retain(void* ptr) {
    if (ptr == nullptr) return;
    auto* h = reinterpret_cast<Header*>(reinterpret_cast<char*>(ptr) - sizeof(Header));
    h->refcount++;
}

void release(void* ptr) {
    if (ptr == nullptr) return;
    auto* h = reinterpret_cast<Header*>(reinterpret_cast<char*>(ptr) - sizeof(Header));
    if (--h->refcount == 0) manual::free(h);
}

} // namespace agn::memory::arc
