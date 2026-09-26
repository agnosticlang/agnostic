// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

#include <stdint.h>

extern "C" {

void agn_rt_print_int(int64_t value);
void agn_rt_println_int(int64_t value);
void agn_rt_print_bool(int64_t value);
void agn_rt_println_bool(int64_t value);
void agn_rt_print_str(const char* s);
void agn_rt_println_str(const char* s);
void agn_rt_print_char(int64_t c);
int64_t agn_rt_read_int();
int64_t agn_rt_read_char();
int64_t agn_rt_read_line(char* buf, int64_t maxlen);
void agn_rt_flush();

uint64_t agn_rt_strlen(const char* s);
int64_t agn_rt_strcmp(const char* a, const char* b);
void agn_rt_memcpy(char* dest, const char* src, uint64_t len);
uint64_t agn_rt_format_int(char* buf, int64_t value, int64_t width, int64_t padZero);
uint64_t agn_rt_format_hex(char* buf, uint64_t value, int64_t width, int64_t padZero, int64_t upper);

} // extern "C"
