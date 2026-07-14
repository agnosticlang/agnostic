// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

// TODO: FreeBSD syscall implementation (see platform/linux/platform.hpp for the interface to fill in)

namespace agn::platform {

[[noreturn]] void exitProcess(int code);
long readFd(int fd, void* buf, unsigned long count);
long writeFd(int fd, const void* buf, unsigned long count);
void* mapAnonymous(unsigned long size);
void unmap(void* ptr, unsigned long size);

} // namespace agn::platform
