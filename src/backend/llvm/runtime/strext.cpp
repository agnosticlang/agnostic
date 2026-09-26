// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include <stdint.h>

extern "C" uint64_t agn_rt_strlen(const char* s);
extern "C" void* agn_rt_alloc(uint64_t size);

namespace {

char toUpperChar(char c) { return (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c; }
char toLowerChar(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

} // namespace

extern "C" int64_t agn_rt_index_of(const char* s, const char* sub) {
    uint64_t slen = agn_rt_strlen(s);
    uint64_t sublen = agn_rt_strlen(sub);
    if (sublen == 0) return 0;
    if (sublen > slen) return -1;
    for (uint64_t i = 0; i + sublen <= slen; i++) {
        uint64_t j = 0;
        while (j < sublen && s[i + j] == sub[j]) j++;
        if (j == sublen) return static_cast<int64_t>(i);
    }
    return -1;
}

extern "C" int64_t agn_rt_contains(const char* s, const char* sub) { return agn_rt_index_of(s, sub) >= 0 ? 1 : 0; }

extern "C" int64_t agn_rt_starts_with(const char* s, const char* prefix) {
    uint64_t i = 0;
    while (prefix[i] != '\0') {
        if (s[i] != prefix[i]) return 0;
        i++;
    }
    return 1;
}

extern "C" int64_t agn_rt_ends_with(const char* s, const char* suffix) {
    uint64_t slen = agn_rt_strlen(s);
    uint64_t suflen = agn_rt_strlen(suffix);
    if (suflen > slen) return 0;
    for (uint64_t i = 0; i < suflen; i++) {
        if (s[slen - suflen + i] != suffix[i]) return 0;
    }
    return 1;
}

extern "C" int64_t agn_rt_char_at(const char* s, int64_t index) {
    uint64_t slen = agn_rt_strlen(s);
    if (index < 0 || static_cast<uint64_t>(index) >= slen) return -1;
    return static_cast<unsigned char>(s[index]);
}

extern "C" char* agn_rt_substr(const char* s, int64_t start, int64_t len) {
    int64_t slen = static_cast<int64_t>(agn_rt_strlen(s));
    if (start < 0) start = 0;
    if (start > slen) start = slen;
    if (len < 0) len = 0;
    if (start + len > slen) len = slen - start;

    char* buf = static_cast<char*>(agn_rt_alloc(static_cast<uint64_t>(len) + 1));
    for (int64_t i = 0; i < len; i++) buf[i] = s[start + i];
    buf[len] = '\0';
    return buf;
}

extern "C" char* agn_rt_to_upper(const char* s) {
    uint64_t len = agn_rt_strlen(s);
    char* buf = static_cast<char*>(agn_rt_alloc(len + 1));
    for (uint64_t i = 0; i < len; i++) buf[i] = toUpperChar(s[i]);
    buf[len] = '\0';
    return buf;
}

extern "C" char* agn_rt_to_lower(const char* s) {
    uint64_t len = agn_rt_strlen(s);
    char* buf = static_cast<char*>(agn_rt_alloc(len + 1));
    for (uint64_t i = 0; i < len; i++) buf[i] = toLowerChar(s[i]);
    buf[len] = '\0';
    return buf;
}
