// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <cstdint>

namespace agn::backend::nvm {

constexpr uint8_t HALT = 0x00;
constexpr uint8_t NOP = 0x01;
constexpr uint8_t PUSH = 0x02;
constexpr uint8_t POP = 0x04;
constexpr uint8_t DUP = 0x05;
constexpr uint8_t SWAP = 0x06;

constexpr uint8_t ADD = 0x10;
constexpr uint8_t SUB = 0x11;
constexpr uint8_t MUL = 0x12;
constexpr uint8_t DIV = 0x13;
constexpr uint8_t MOD = 0x14;

constexpr uint8_t CMP = 0x20;
constexpr uint8_t EQ = 0x21;
constexpr uint8_t NEQ = 0x22;
constexpr uint8_t GT = 0x23;
constexpr uint8_t LT = 0x24;

constexpr uint8_t JMP = 0x30;
constexpr uint8_t JZ = 0x31;
constexpr uint8_t JNZ = 0x32;
constexpr uint8_t CALL = 0x33;
constexpr uint8_t RET = 0x34;
constexpr uint8_t ENTER = 0x35;
constexpr uint8_t LEAVE = 0x36;
constexpr uint8_t LOAD_ARG = 0x37;
constexpr uint8_t STORE_ARG = 0x38;

constexpr uint8_t LOAD = 0x40;
constexpr uint8_t STORE = 0x41;
constexpr uint8_t LOAD_REL = 0x42;
constexpr uint8_t STORE_REL = 0x43;
constexpr uint8_t LOAD_ABS = 0x44;
constexpr uint8_t STORE_ABS = 0x45;
constexpr uint8_t LOAD_HEAP = 0x46;
constexpr uint8_t STORE_HEAP = 0x47;

constexpr uint8_t SYSCALL = 0x50;
constexpr uint8_t BREAK = 0x51;

constexpr uint8_t AND = 0x60;
constexpr uint8_t OR = 0x61;
constexpr uint8_t XOR = 0x62;
constexpr uint8_t NOT = 0x63;
constexpr uint8_t SHL = 0x64;
constexpr uint8_t SHR = 0x65;
constexpr uint8_t SAR = 0x66;

constexpr uint8_t SYS_EXIT = 0x00;
constexpr uint8_t SYS_SPAWN = 0x01;
constexpr uint8_t SYS_OPEN = 0x10;
constexpr uint8_t SYS_READ = 0x12;
constexpr uint8_t SYS_WRITE = 0x13;
constexpr uint8_t SYS_REMOVE = 0x15;
constexpr uint8_t SYS_SBRK = 0x20;

constexpr uint8_t CAP_FS_READ = 0x0001;
constexpr uint8_t CAP_FS_WRITE = 0x0002;
constexpr uint8_t CAP_FS_CREATE = 0x0003;
constexpr uint8_t CAP_FS_DELETE = 0x0004;
constexpr uint8_t CAP_MEM_MGMT = 0x0005;
constexpr uint8_t CAP_DRV_ACCESS = 0x0006;
constexpr uint8_t CAP_PROC_MGMT = 0x0007;
constexpr uint8_t CAP_CAPS_MGMT = 0x0008;

} // namespace agn::backend::nvm
