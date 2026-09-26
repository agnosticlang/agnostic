// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "platform/windows/platform.hpp"

extern "C" {

[[noreturn]] __declspec(dllimport) void ExitProcess(uint32_t exitCode);
__declspec(dllimport) void* GetStdHandle(uint32_t stdHandle);
__declspec(dllimport) int ReadFile(void* file, void* buffer, uint32_t toRead, uint32_t* read, void* overlapped);
__declspec(dllimport) int WriteFile(void* file, const void* buffer, uint32_t toWrite, uint32_t* written,
                                    void* overlapped);
__declspec(dllimport) void* CreateFileW(const wchar_t* name, uint32_t access, uint32_t shareMode, void* security,
                                        uint32_t disposition, uint32_t flags, void* templateFile);
__declspec(dllimport) int CloseHandle(void* object);
__declspec(dllimport) uint32_t GetLastError();
__declspec(dllimport) void* VirtualAlloc(void* address, uint64_t size, uint32_t type, uint32_t protect);
__declspec(dllimport) int VirtualFree(void* address, uint64_t size, uint32_t type);
__declspec(dllimport) wchar_t* GetCommandLineW();
__declspec(dllimport) int MultiByteToWideChar(uint32_t codePage, uint32_t flags, const char* src, int srcLen,
                                              wchar_t* dst, int dstLen);
__declspec(dllimport) int WideCharToMultiByte(uint32_t codePage, uint32_t flags, const wchar_t* src, int srcLen,
                                              char* dst, int dstLen, const char* defaultChar, int* usedDefault);

int _fltused = 0;

int main();

}

namespace {

constexpr uint32_t kStdInputHandle = static_cast<uint32_t>(-10);
constexpr uint32_t kStdOutputHandle = static_cast<uint32_t>(-11);
constexpr uint32_t kStdErrorHandle = static_cast<uint32_t>(-12);
constexpr uint32_t kGenericRead = 0x80000000;
constexpr uint32_t kGenericWrite = 0x40000000;
constexpr uint32_t kFileShareRead = 0x1;
constexpr uint32_t kFileShareWrite = 0x2;
constexpr uint32_t kCreateAlways = 2;
constexpr uint32_t kOpenExisting = 3;
constexpr uint32_t kFileAttributeNormal = 0x80;
constexpr uint32_t kMemCommitReserve = 0x3000;
constexpr uint32_t kMemRelease = 0x8000;
constexpr uint32_t kPageReadWrite = 0x04;
constexpr uint32_t kCodePageUtf8 = 65001;
constexpr uint32_t kErrorBrokenPipe = 109;
constexpr intptr_t kInvalidHandle = -1;

int64_t argcValue = 0;
char** argvValue = nullptr;

void* handleFor(int fd) {
    if (fd == 0) return GetStdHandle(kStdInputHandle);
    if (fd == 1) return GetStdHandle(kStdOutputHandle);
    if (fd == 2) return GetStdHandle(kStdErrorHandle);
    return reinterpret_cast<void*>(static_cast<intptr_t>(fd));
}

uint32_t clampToDword(uint64_t n) { return n > 0xFFFFFFFFu ? 0xFFFFFFFFu : static_cast<uint32_t>(n); }

int64_t openFile(const char* path, uint32_t access, uint32_t shareMode, uint32_t disposition) {
    int wideLen = MultiByteToWideChar(kCodePageUtf8, 0, path, -1, nullptr, 0);
    if (wideLen <= 0) return -1;
    uint64_t wideBytes = static_cast<uint64_t>(wideLen) * sizeof(wchar_t);
    auto* widePath = static_cast<wchar_t*>(agn::platform::mapAnonymous(wideBytes));
    if (widePath == nullptr) return -1;
    MultiByteToWideChar(kCodePageUtf8, 0, path, -1, widePath, wideLen);
    void* handle = CreateFileW(widePath, access, shareMode, nullptr, disposition, kFileAttributeNormal, nullptr);
    agn::platform::unmap(widePath, wideBytes);
    if (reinterpret_cast<intptr_t>(handle) == kInvalidHandle) return -1;
    return reinterpret_cast<intptr_t>(handle);
}

bool isArgSeparator(wchar_t c) { return c == L' ' || c == L'\t'; }

const wchar_t* parseProgramName(const wchar_t* p, wchar_t*& out) {
    if (*p == L'"') {
        p++;
        while (*p != 0 && *p != L'"') *out++ = *p++;
        if (*p == L'"') p++;
    } else {
        while (*p != 0 && !isArgSeparator(*p)) *out++ = *p++;
    }
    *out++ = 0;
    return p;
}

const wchar_t* parseArg(const wchar_t* p, wchar_t*& out) {
    bool inQuotes = false;
    for (;;) {
        uint64_t backslashes = 0;
        while (*p == L'\\') {
            backslashes++;
            p++;
        }
        if (*p == L'"') {
            for (uint64_t i = 0; i < backslashes / 2; i++) *out++ = L'\\';
            if (backslashes % 2 == 1) {
                *out++ = L'"';
            } else if (inQuotes && p[1] == L'"') {
                *out++ = L'"';
                p++;
            } else {
                inQuotes = !inQuotes;
            }
            p++;
            continue;
        }
        for (uint64_t i = 0; i < backslashes; i++) *out++ = L'\\';
        if (*p == 0 || (!inQuotes && isArgSeparator(*p))) break;
        *out++ = *p++;
    }
    *out++ = 0;
    return p;
}

void parseCommandLine() {
    const wchar_t* commandLine = GetCommandLineW();
    uint64_t len = 0;
    while (commandLine[len] != 0) len++;

    uint64_t maxArgs = len + 1;
    uint64_t argvBytes = (maxArgs + 1) * sizeof(char*);
    uint64_t wideBytes = (len + maxArgs) * sizeof(wchar_t);
    uint64_t utf8Bytes = 3 * len + maxArgs;
    auto* memory = static_cast<char*>(agn::platform::mapAnonymous(argvBytes + wideBytes + utf8Bytes));
    if (memory == nullptr) return;
    auto** argv = reinterpret_cast<char**>(memory);
    auto* wide = reinterpret_cast<wchar_t*>(memory + argvBytes);
    char* utf8 = memory + argvBytes + wideBytes;

    wchar_t* out = wide;
    const wchar_t* p = parseProgramName(commandLine, out);
    int64_t argc = 1;
    for (;;) {
        while (isArgSeparator(*p)) p++;
        if (*p == 0) break;
        p = parseArg(p, out);
        argc++;
    }

    const wchar_t* arg = wide;
    char* utf8End = utf8 + utf8Bytes;
    for (int64_t i = 0; i < argc; i++) {
        argv[i] = utf8;
        int written = WideCharToMultiByte(kCodePageUtf8, 0, arg, -1, utf8, static_cast<int>(utf8End - utf8), nullptr,
                                          nullptr);
        utf8 += written;
        while (*arg != 0) arg++;
        arg++;
    }
    argv[argc] = nullptr;
    argvValue = argv;
    argcValue = argc;
}

} // namespace

extern "C" [[noreturn]] void agn_start() {
    parseCommandLine();
    agn::platform::exitProcess(main());
}

namespace agn::platform {

void exitProcess(int code) { ExitProcess(static_cast<uint32_t>(code)); }

int64_t readFd(int fd, void* buf, uint64_t count) {
    uint32_t read = 0;
    if (ReadFile(handleFor(fd), buf, clampToDword(count), &read, nullptr)) return read;
    return GetLastError() == kErrorBrokenPipe ? 0 : -1;
}

int64_t writeFd(int fd, const void* buf, uint64_t count) {
    uint32_t written = 0;
    if (!WriteFile(handleFor(fd), buf, clampToDword(count), &written, nullptr)) return -1;
    return written;
}

void* mapAnonymous(uint64_t size) { return VirtualAlloc(nullptr, size, kMemCommitReserve, kPageReadWrite); }

void unmap(void* ptr, uint64_t) { VirtualFree(ptr, 0, kMemRelease); }

int64_t openRead(const char* path) {
    return openFile(path, kGenericRead, kFileShareRead | kFileShareWrite, kOpenExisting);
}

int64_t openCreate(const char* path) { return openFile(path, kGenericWrite, kFileShareRead, kCreateAlways); }

int64_t closeFd(int fd) { return CloseHandle(handleFor(fd)) ? 0 : -1; }

int64_t argCount() { return argcValue; }

const char* argAt(int64_t index) {
    if (index < 0 || index >= argcValue) return nullptr;
    return argvValue[index];
}

} // namespace agn::platform
