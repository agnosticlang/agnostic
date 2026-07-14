// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/arc/arc.hpp"

extern "C" void* agn_rt_alloc(unsigned long size) { return agn::memory::arc::alloc(size); }
extern "C" void agn_rt_retain(void* ptr) { agn::memory::arc::retain(ptr); }
extern "C" void agn_rt_release(void* ptr) { agn::memory::arc::release(ptr); }
