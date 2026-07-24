// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "memory/orc/orc.hpp"

extern "C" void* agn_rt_alloc(unsigned long size) { return agn::memory::orc::alloc(size); }
extern "C" void agn_rt_retain(void*) {}
extern "C" void agn_rt_release(void*) {}
extern "C" void agn_rt_orc_enter() { agn::memory::orc::enterRegion(); }
extern "C" void agn_rt_orc_exit() { agn::memory::orc::exitRegion(); }
