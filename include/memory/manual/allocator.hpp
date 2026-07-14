// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

namespace agn::memory::manual {

void* alloc(unsigned long size);
void free(void* ptr);

} // namespace agn::memory::manual
