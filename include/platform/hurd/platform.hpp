// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdint.h>

namespace agn::platform {

[[noreturn]] void exitProcess(int code);
int64_t readFd(int fd, void* buf, uint64_t count);
int64_t writeFd(int fd, const void* buf, uint64_t count);
void* mapAnonymous(uint64_t size);
void unmap(void* ptr, uint64_t size);
int64_t openRead(const char* path);
int64_t openCreate(const char* path);
int64_t closeFd(int fd);
int64_t argCount();
const char* argAt(int64_t index);

} // namespace agn::platform
