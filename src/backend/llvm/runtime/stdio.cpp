// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "backend/llvm/runtime/stdio.hpp"
#include "platform/linux/platform.hpp"

namespace {

void writeAll(const char* p, unsigned long len) { agn::platform::writeFd(1, p, len); }

long readByte() {
    char c;
    long n = agn::platform::readFd(0, &c, 1);
    if (n <= 0) return -1;
    return static_cast<unsigned char>(c);
}

} // namespace

extern "C" unsigned long agn_rt_strlen(const char* s) {
    unsigned long n = 0;
    while (s[n] != '\0') n++;
    return n;
}

extern "C" long agn_rt_strcmp(const char* a, const char* b) {
    unsigned long i = 0;
    while (a[i] != '\0' && a[i] == b[i]) i++;
    unsigned char ca = static_cast<unsigned char>(a[i]);
    unsigned char cb = static_cast<unsigned char>(b[i]);
    if (ca == cb) return 0;
    return ca < cb ? -1 : 1;
}

extern "C" void agn_rt_memcpy(char* dest, const char* src, unsigned long len) {
    for (unsigned long i = 0; i < len; i++) dest[i] = src[i];
}

extern "C" unsigned long agn_rt_format_int(char* buf, long value, long width, long padZero) {
    char tmp[24];
    int pos = 24;
    bool neg = value < 0;
    unsigned long v = neg ? static_cast<unsigned long>(-(value + 1)) + 1 : static_cast<unsigned long>(value);
    if (v == 0) tmp[--pos] = '0';
    while (v > 0) {
        tmp[--pos] = static_cast<char>('0' + (v % 10));
        v /= 10;
    }
    if (neg) tmp[--pos] = '-';

    long len = 24 - pos;
    long padCount = width > len ? width - len : 0;
    long out = 0;
    for (long i = 0; i < padCount; i++) buf[out++] = padZero ? '0' : ' ';
    for (int i = pos; i < 24; i++) buf[out++] = tmp[i];
    return static_cast<unsigned long>(out);
}

extern "C" unsigned long agn_rt_format_hex(char* buf, unsigned long value, long width, long padZero, long upper) {
    char tmp[16];
    int pos = 16;
    const char* digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    if (value == 0) tmp[--pos] = '0';
    while (value > 0) {
        tmp[--pos] = digits[value & 0xF];
        value >>= 4;
    }

    long len = 16 - pos;
    long padCount = width > len ? width - len : 0;
    long out = 0;
    for (long i = 0; i < padCount; i++) buf[out++] = padZero ? '0' : ' ';
    for (int i = pos; i < 16; i++) buf[out++] = tmp[i];
    return static_cast<unsigned long>(out);
}

extern "C" void agn_rt_print_int(long value) {
    char buf[32];
    unsigned long len = agn_rt_format_int(buf, value, 0, 0);
    writeAll(buf, len);
}

extern "C" void agn_rt_println_int(long value) {
    agn_rt_print_int(value);
    writeAll("\n", 1);
}

extern "C" void agn_rt_print_str(const char* s) { writeAll(s, agn_rt_strlen(s)); }

extern "C" void agn_rt_println_str(const char* s) {
    writeAll(s, agn_rt_strlen(s));
    writeAll("\n", 1);
}

extern "C" void agn_rt_print_char(long c) {
    char ch = static_cast<char>(c);
    writeAll(&ch, 1);
}

extern "C" long agn_rt_read_int() {
    long c = readByte();
    while (c == ' ' || c == '\t' || c == '\n' || c == '\r') c = readByte();

    bool neg = false;
    if (c == '-') { neg = true; c = readByte(); }
    else if (c == '+') { c = readByte(); }

    long value = 0;
    while (c >= '0' && c <= '9') {
        value = value * 10 + (c - '0');
        c = readByte();
    }
    return neg ? -value : value;
}

extern "C" long agn_rt_read_char() { return readByte(); }

extern "C" long agn_rt_read_line(char* buf, long maxlen) {
    long i = 0;
    while (i < maxlen - 1) {
        long c = readByte();
        if (c < 0 || c == '\n') break;
        buf[i++] = static_cast<char>(c);
    }
    buf[i] = '\0';
    return i;
}

extern "C" void agn_rt_flush() {}
