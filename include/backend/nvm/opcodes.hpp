// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <cstdint>

namespace agn::backend::nvm {

constexpr uint8_t PUSH32 = 0x02;
constexpr uint8_t POP = 0x04;
constexpr uint8_t SWAP = 0x06;

constexpr uint8_t ADD = 0x10;
constexpr uint8_t SUB = 0x11;
constexpr uint8_t MUL = 0x12;
constexpr uint8_t DIV = 0x13;
constexpr uint8_t MOD = 0x14;

constexpr uint8_t EQ = 0x21;
constexpr uint8_t NEQ = 0x22;
constexpr uint8_t GT = 0x23;
constexpr uint8_t LT = 0x24;

constexpr uint8_t JMP32 = 0x30;
constexpr uint8_t JZ32 = 0x31;
constexpr uint8_t JNZ32 = 0x32;
constexpr uint8_t CALL32 = 0x33;
constexpr uint8_t RET = 0x34;

constexpr uint8_t LOAD = 0x40;
constexpr uint8_t STORE = 0x41;
constexpr uint8_t LOAD_ABS = 0x44;
constexpr uint8_t STORE_ABS = 0x45;

constexpr uint8_t SYSCALL = 0x50;

constexpr uint8_t SYSCALL_EXIT = 0x00;
constexpr uint8_t SYSCALL_EXEC = 0x01;
constexpr uint8_t SYSCALL_OPEN = 0x02;
constexpr uint8_t SYSCALL_READ = 0x03;
constexpr uint8_t SYSCALL_WRITE = 0x04;
constexpr uint8_t SYSCALL_CREATE = 0x05;
constexpr uint8_t SYSCALL_DELETE = 0x06;
constexpr uint8_t SYSCALL_CAP_CHECK = 0x07;
constexpr uint8_t SYSCALL_CAP_SPAWN = 0x08;
constexpr uint8_t SYSCALL_MSG_SEND = 0x0A;
constexpr uint8_t SYSCALL_MSG_RECEIVE = 0x0B;
constexpr uint8_t SYSCALL_PORT_IN_BYTE = 0x0C;
constexpr uint8_t SYSCALL_PORT_OUT_BYTE = 0x0D;
constexpr uint8_t SYSCALL_PRINT = 0x0E;

} // namespace agn::backend::nvm
