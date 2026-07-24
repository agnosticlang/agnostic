// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#pragma once

extern "C" {

void agn_rt_print_int(long value);
void agn_rt_println_int(long value);
void agn_rt_print_str(const char* s);
void agn_rt_println_str(const char* s);
void agn_rt_print_char(long c);
long agn_rt_read_int();
long agn_rt_read_char();
long agn_rt_read_line(char* buf, long maxlen);
void agn_rt_flush();

unsigned long agn_rt_strlen(const char* s);
long agn_rt_strcmp(const char* a, const char* b);
void agn_rt_memcpy(char* dest, const char* src, unsigned long len);
unsigned long agn_rt_format_int(char* buf, long value, long width, long padZero);
unsigned long agn_rt_format_hex(char* buf, unsigned long value, long width, long padZero, long upper);

} // extern "C"
