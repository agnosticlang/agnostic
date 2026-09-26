// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#if defined(AGN_TARGET_FREEBSD)
#include "platform/freebsd/platform.hpp"
#elif defined(AGN_TARGET_WINDOWS)
#include "platform/windows/platform.hpp"
#elif defined(AGN_TARGET_HURD)
#include "platform/hurd/platform.hpp"
#else
#include "platform/linux/platform.hpp"
#endif

extern "C" int64_t agn_rt_argc() { return agn::platform::argCount(); }

extern "C" const char* agn_rt_argv(int64_t index) { return agn::platform::argAt(index); }

extern "C" int64_t agn_rt_open_read(const char* path) { return agn::platform::openRead(path); }

extern "C" int64_t agn_rt_open_create(const char* path) { return agn::platform::openCreate(path); }

extern "C" int64_t agn_rt_close(int64_t fd) { return agn::platform::closeFd(static_cast<int>(fd)); }

extern "C" int64_t agn_rt_read_fd(int64_t fd, char* buf, int64_t maxlen) {
    return agn::platform::readFd(static_cast<int>(fd), buf, static_cast<uint64_t>(maxlen));
}

extern "C" int64_t agn_rt_write_fd(int64_t fd, const char* buf, int64_t len) {
    return agn::platform::writeFd(static_cast<int>(fd), buf, static_cast<uint64_t>(len));
}

extern "C" [[noreturn]] void agn_rt_exit(int64_t code) { agn::platform::exitProcess(static_cast<int>(code)); }
