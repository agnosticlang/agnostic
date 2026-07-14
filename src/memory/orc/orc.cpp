// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/orc/orc.hpp"
#include "memory/manual/allocator.hpp"

namespace agn::memory::orc {

namespace {

struct Node {
    void* ptr;
    Node* next;
};

struct Region {
    Region* parent;
    Node* allocations;
};

Region* current = nullptr;

} // namespace

void enterRegion() {
    auto* r = reinterpret_cast<Region*>(manual::alloc(sizeof(Region)));
    r->parent = current;
    r->allocations = nullptr;
    current = r;
}

void* alloc(unsigned long size) {
    if (current == nullptr) enterRegion();

    void* ptr = manual::alloc(size);
    if (ptr == nullptr) return nullptr;

    auto* node = reinterpret_cast<Node*>(manual::alloc(sizeof(Node)));
    node->ptr = ptr;
    node->next = current->allocations;
    current->allocations = node;
    return ptr;
}

void exitRegion() {
    if (current == nullptr) return;
    for (Node* node = current->allocations; node != nullptr;) {
        Node* next = node->next;
        manual::free(node->ptr);
        manual::free(node);
        node = next;
    }
    Region* parent = current->parent;
    manual::free(current);
    current = parent;
}

} // namespace agn::memory::orc
