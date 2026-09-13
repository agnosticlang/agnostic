// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

namespace agn::memory::manual {

void* alloc(unsigned long size);
void free(void* ptr);

} // namespace agn::memory::manual
