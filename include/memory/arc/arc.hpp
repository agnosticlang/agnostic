// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

namespace agn::memory::arc {

void* alloc(unsigned long size);
void retain(void* ptr);
void release(void* ptr);

} // namespace agn::memory::arc
