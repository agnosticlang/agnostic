// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

namespace agn::memory::orc {

void enterRegion();
void exitRegion();
void* alloc(unsigned long size);

} // namespace agn::memory::orc
