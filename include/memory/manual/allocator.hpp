// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdint.h>

namespace agn::memory::manual {

void* alloc(uint64_t size);
void free(void* ptr);

} // namespace agn::memory::manual
