// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdint.h>

namespace agn::memory::orc {

void enterRegion();
void exitRegion();
void* alloc(uint64_t size);

} // namespace agn::memory::orc
