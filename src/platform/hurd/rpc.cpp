// SPDX-License-Identifier: GPL-3.0-or-later
// SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#include "platform/hurd/platform.hpp"

extern "C" {

int32_t agn_mach_msg_trap(void* msg, int32_t option, uint32_t sendSize, uint32_t receiveSize, uint32_t receiveName,
                          uint32_t timeout, uint32_t notify);
uint32_t agn_mach_reply_port();
uint32_t agn_mach_task_self();
int32_t agn_vm_allocate(uint32_t task, uint64_t* address, uint64_t size, int32_t anywhere);
int32_t agn_vm_deallocate(uint32_t task, uint64_t address, uint64_t size);
int32_t agn_task_terminate(uint32_t task);
int32_t agn_mach_port_deallocate(uint32_t task, uint32_t name);

int main();

}

namespace {

constexpr int32_t kSendAndReceive = 0x3;
constexpr uint32_t kCopySend = 19;
constexpr uint32_t kMakeSendOnce = 21;
constexpr uint32_t kTypeInteger32 = 2;
constexpr uint32_t kTypeChar = 8;
constexpr uint32_t kTypeInteger64 = 11;
constexpr uint32_t kTypeStringC = 12;
constexpr uint32_t kTypeInline = 1u << 29;
constexpr int32_t kReplyIdOffset = 100;
constexpr uint64_t kStringCapacity = 1024;
constexpr uint64_t kMaxInlineData = 2048;

constexpr int32_t kTaskGetSpecialPort = 2058;
constexpr int32_t kFileSetSize = 20006;
constexpr int32_t kDirLookup = 20018;
constexpr int32_t kIoWrite = 21000;
constexpr int32_t kIoRead = 21001;
constexpr int32_t kProcMarkExit = 24025;
constexpr int32_t kExecStartupGetInfo = 30500;

constexpr int32_t kTaskBootstrapPort = 4;
constexpr uint32_t kExecStackArgs = 0x10;
constexpr uint32_t kInitPortCwdir = 0;
constexpr uint32_t kInitPortCrdir = 1;
constexpr uint32_t kInitPortProc = 3;
constexpr uint32_t kRetryNormal = 1;
constexpr uint32_t kRetryMagical = 3;
constexpr int kMaxLookupHops = 40;
constexpr int32_t kOpenRead = 0x1;
constexpr int32_t kOpenWrite = 0x2;
constexpr int32_t kOpenCreate = 0x10;
constexpr int32_t kCreateMode = 0644;
constexpr int kMaxDescriptors = 1024;

struct MessageHeader {
    uint32_t bits;
    uint32_t size;
    uint64_t remotePort;
    uint64_t localPort;
    uint32_t seqno;
    int32_t id;
};
static_assert(sizeof(MessageHeader) == 32);

struct TypeDescriptor {
    uint32_t word;
    uint32_t number;
};
static_assert(sizeof(TypeDescriptor) == 8);

struct Item {
    uint32_t sizeBits;
    uint32_t number;
    bool isInline;
    char* data;
};

struct Message {
    uint32_t offset;
    uint32_t end;
};

alignas(8) char messageBuffer[16384];
uint32_t taskSelf = 0;
uint32_t replyPort = 0;
uint32_t cwdirPort = 0;
uint32_t crdirPort = 0;
uint32_t procPort = 0;
uint32_t descriptorPorts[kMaxDescriptors];
int64_t argcValue = 0;
char** argvValue = nullptr;

uint64_t roundUp8(uint64_t n) { return (n + 7) & ~static_cast<uint64_t>(7); }

template <typename T>
T load(const char* p) {
    T value;
    __builtin_memcpy(&value, p, sizeof(T));
    return value;
}

template <typename T>
void store(char* p, T value) {
    __builtin_memcpy(p, &value, sizeof(T));
}

void copyBytes(char* dst, const char* src, uint64_t n) {
    for (uint64_t i = 0; i < n; i++) dst[i] = src[i];
}

Message beginRequest() { return Message{sizeof(MessageHeader), 0}; }

void putDescriptor(Message& m, uint32_t name, uint32_t sizeBits, uint32_t number) {
    store(messageBuffer + m.offset, TypeDescriptor{name | (sizeBits << 8) | kTypeInline, number});
    m.offset += sizeof(TypeDescriptor);
}

void putInt32(Message& m, int32_t value) {
    putDescriptor(m, kTypeInteger32, 32, 1);
    store(messageBuffer + m.offset, value);
    store(messageBuffer + m.offset + 4, static_cast<uint32_t>(0));
    m.offset += 8;
}

void putInt64(Message& m, int64_t value) {
    putDescriptor(m, kTypeInteger64, 64, 1);
    store(messageBuffer + m.offset, value);
    m.offset += 8;
}

void putBytes(Message& m, const char* data, uint64_t count) {
    putDescriptor(m, kTypeChar, 8, static_cast<uint32_t>(count));
    uint64_t padded = roundUp8(count);
    copyBytes(messageBuffer + m.offset, data, count);
    for (uint64_t i = count; i < padded; i++) messageBuffer[m.offset + i] = 0;
    m.offset += static_cast<uint32_t>(padded);
}

bool putString(Message& m, const char* text) {
    putDescriptor(m, kTypeStringC, 8, kStringCapacity);
    char* field = messageBuffer + m.offset;
    uint64_t i = 0;
    for (; text[i] != '\0'; i++) {
        if (i + 1 >= kStringCapacity) return false;
        field[i] = text[i];
    }
    for (; i < kStringCapacity; i++) field[i] = '\0';
    m.offset += kStringCapacity;
    return true;
}

bool nextItem(Message& m, Item& item) {
    if (m.offset + sizeof(TypeDescriptor) > m.end) return false;
    auto descriptor = load<TypeDescriptor>(messageBuffer + m.offset);
    item.sizeBits = (descriptor.word >> 8) & 0xffff;
    item.number = descriptor.number;
    item.isInline = (descriptor.word & kTypeInline) != 0;
    m.offset += sizeof(TypeDescriptor);
    if (item.isInline) {
        item.data = messageBuffer + m.offset;
        m.offset += static_cast<uint32_t>(roundUp8(static_cast<uint64_t>(item.number) * item.sizeBits / 8));
    } else {
        item.data = load<char*>(messageBuffer + m.offset);
        m.offset += sizeof(char*);
    }
    return m.offset <= m.end;
}

int64_t integerValue(const Item& item) {
    if (item.sizeBits == 64) return load<int64_t>(item.data);
    return load<int32_t>(item.data);
}

uint32_t portAt(const Item& item, uint32_t index) {
    return load<uint32_t>(item.data + static_cast<uint64_t>(index) * (item.sizeBits / 8));
}

void releaseOutOfLine(const Item& item) {
    if (item.isInline || item.data == nullptr) return;
    agn_vm_deallocate(taskSelf, reinterpret_cast<uint64_t>(item.data),
                      static_cast<uint64_t>(item.number) * item.sizeBits / 8);
}

int32_t call(Message& m, uint32_t destination, int32_t id) {
    store(messageBuffer, MessageHeader{kCopySend | (kMakeSendOnce << 8), m.offset, destination, replyPort, 0, id});
    int32_t result =
        agn_mach_msg_trap(messageBuffer, kSendAndReceive, m.offset, sizeof(messageBuffer), replyPort, 0, 0);
    if (result != 0) return result;
    auto reply = load<MessageHeader>(messageBuffer);
    if (reply.id != id + kReplyIdOffset) return -1;

    m.offset = sizeof(MessageHeader);
    m.end = reply.size;
    Item returnCode;
    if (!nextItem(m, returnCode)) return -1;
    return static_cast<int32_t>(integerValue(returnCode));
}

uint32_t bootstrapPort() {
    Message m = beginRequest();
    putInt32(m, kTaskBootstrapPort);
    Item port;
    if (call(m, taskSelf, kTaskGetSpecialPort) != 0 || !nextItem(m, port)) return 0;
    return portAt(port, 0);
}

void setArguments(const Item& args) {
    uint64_t length = args.number;
    int64_t count = 0;
    for (uint64_t i = 0; i < length; i++) {
        if (args.data[i] == '\0') count++;
    }
    uint64_t vectorBytes = static_cast<uint64_t>(count + 1) * sizeof(char*);
    auto* memory = static_cast<char*>(agn::platform::mapAnonymous(vectorBytes + length + 1));
    if (memory == nullptr) return;
    auto** argv = reinterpret_cast<char**>(memory);
    char* strings = memory + vectorBytes;
    copyBytes(strings, args.data, length);

    char* start = strings;
    for (int64_t i = 0; i < count; i++) {
        argv[i] = start;
        while (*start != '\0') start++;
        start++;
    }
    argv[count] = nullptr;
    argvValue = argv;
    argcValue = count;
}

bool execStartup(uint32_t bootstrap, uint32_t& flags) {
    Message m = beginRequest();
    if (call(m, bootstrap, kExecStartupGetInfo) != 0) return false;

    Item item;
    for (int i = 0; i < 5; i++) {
        if (!nextItem(m, item)) return false;
    }
    if (!nextItem(m, item)) return false;
    flags = static_cast<uint32_t>(integerValue(item));

    Item args, env, dtable, ports, ints;
    if (!nextItem(m, args) || !nextItem(m, env) || !nextItem(m, dtable) || !nextItem(m, ports) ||
        !nextItem(m, ints)) {
        return false;
    }

    setArguments(args);
    for (uint32_t fd = 0; fd < dtable.number && fd < kMaxDescriptors; fd++) descriptorPorts[fd] = portAt(dtable, fd);
    if (ports.number > kInitPortCwdir) cwdirPort = portAt(ports, kInitPortCwdir);
    if (ports.number > kInitPortCrdir) crdirPort = portAt(ports, kInitPortCrdir);
    if (ports.number > kInitPortProc) procPort = portAt(ports, kInitPortProc);

    releaseOutOfLine(args);
    releaseOutOfLine(env);
    releaseOutOfLine(dtable);
    releaseOutOfLine(ports);
    releaseOutOfLine(ints);
    return true;
}

uint32_t portFor(int fd) {
    if (fd < 0 || fd >= kMaxDescriptors) return 0;
    return descriptorPorts[fd];
}

int64_t allocateDescriptor(uint32_t port) {
    for (int fd = 0; fd < kMaxDescriptors; fd++) {
        if (descriptorPorts[fd] == 0) {
            descriptorPorts[fd] = port;
            return fd;
        }
    }
    agn_mach_port_deallocate(taskSelf, port);
    return -1;
}

const char* skipLeadingSlash(const char* name) {
    if (name[0] != '/' || name[1] == '\0') return name;
    while (name[1] == '/') name++;
    return name[1] == '\0' ? name : name + 1;
}

bool copyName(char* dst, const char* src) {
    for (uint64_t i = 0; i < kStringCapacity; i++) {
        dst[i] = src[i];
        if (src[i] == '\0') return true;
    }
    return false;
}

int32_t dirLookup(uint32_t dir, const char* name, int32_t flags, uint32_t& retryType, char* retryName,
                  uint32_t& result) {
    Message m = beginRequest();
    if (!putString(m, name)) return -1;
    putInt32(m, flags);
    putInt32(m, kCreateMode);
    int32_t error = call(m, dir, kDirLookup);
    if (error != 0) return error;

    Item retry, retryPath, port;
    if (!nextItem(m, retry) || !nextItem(m, retryPath) || !nextItem(m, port)) return -1;
    retryType = static_cast<uint32_t>(integerValue(retry));
    uint64_t length = retryPath.number < kStringCapacity ? retryPath.number : kStringCapacity - 1;
    copyBytes(retryName, retryPath.data, length);
    retryName[length] = '\0';
    result = portAt(port, 0);
    return 0;
}

int32_t fileSetSize(uint32_t file, int64_t size) {
    Message m = beginRequest();
    putInt64(m, size);
    return call(m, file, kFileSetSize);
}

int64_t openFile(const char* path, int32_t flags, bool truncate) {
    char name[kStringCapacity];
    if (!copyName(name, path)) return -1;
    uint32_t dir = path[0] == '/' ? crdirPort : cwdirPort;
    bool ownsDir = false;

    for (int hop = 0; hop < kMaxLookupHops; hop++) {
        uint32_t retryType = 0;
        uint32_t port = 0;
        char retryName[kStringCapacity];
        int32_t error = dirLookup(dir, skipLeadingSlash(name), flags, retryType, retryName, port);
        if (ownsDir) agn_mach_port_deallocate(taskSelf, dir);
        if (error != 0) return -1;

        if (retryType == kRetryNormal && retryName[0] == '\0') {
            if (truncate && fileSetSize(port, 0) != 0) {
                agn_mach_port_deallocate(taskSelf, port);
                return -1;
            }
            return allocateDescriptor(port);
        }
        if (retryType == kRetryNormal) {
            dir = port;
            ownsDir = true;
            copyName(name, retryName);
            continue;
        }
        if (port != 0) agn_mach_port_deallocate(taskSelf, port);
        if (retryType != kRetryMagical || retryName[0] != '/') return -1;
        dir = crdirPort;
        ownsDir = false;
        copyName(name, retryName + 1);
    }
    return -1;
}

int32_t procMarkExit(int32_t status) {
    Message m = beginRequest();
    putInt32(m, status);
    putInt32(m, 0);
    return call(m, procPort, kProcMarkExit);
}

} // namespace

extern "C" [[noreturn]] void agn_hurd_start(uint64_t* initialStack) {
    taskSelf = agn_mach_task_self();
    replyPort = agn_mach_reply_port();

    uint32_t flags = 0;
    uint32_t bootstrap = bootstrapPort();
    bool started = bootstrap != 0 && execStartup(bootstrap, flags);
    if (bootstrap != 0) agn_mach_port_deallocate(taskSelf, bootstrap);
    if (!started || (flags & kExecStackArgs) != 0) {
        argcValue = static_cast<int64_t>(initialStack[0]);
        argvValue = reinterpret_cast<char**>(initialStack + 1);
    }
    agn::platform::exitProcess(main());
}

namespace agn::platform {

void exitProcess(int code) {
    if (procPort != 0) procMarkExit((code & 0xff) << 8);
    for (;;) agn_task_terminate(taskSelf);
}

int64_t readFd(int fd, void* buf, uint64_t count) {
    uint32_t port = portFor(fd);
    if (port == 0) return -1;
    Message m = beginRequest();
    putInt64(m, -1);
    putInt64(m, static_cast<int64_t>(count));
    Item data;
    if (call(m, port, kIoRead) != 0 || !nextItem(m, data)) return -1;
    uint64_t length = data.number < count ? data.number : count;
    copyBytes(static_cast<char*>(buf), data.data, length);
    releaseOutOfLine(data);
    return static_cast<int64_t>(length);
}

int64_t writeFd(int fd, const void* buf, uint64_t count) {
    uint32_t port = portFor(fd);
    if (port == 0) return -1;
    const char* bytes = static_cast<const char*>(buf);
    uint64_t total = 0;
    while (total < count) {
        uint64_t chunk = count - total < kMaxInlineData ? count - total : kMaxInlineData;
        Message m = beginRequest();
        putBytes(m, bytes + total, chunk);
        putInt64(m, -1);
        Item amount;
        if (call(m, port, kIoWrite) != 0 || !nextItem(m, amount)) return total > 0 ? static_cast<int64_t>(total) : -1;
        uint64_t written = static_cast<uint64_t>(integerValue(amount));
        total += written;
        if (written < chunk) break;
    }
    return static_cast<int64_t>(total);
}

void* mapAnonymous(uint64_t size) {
    uint64_t address = 0;
    if (agn_vm_allocate(taskSelf, &address, size, 1) != 0) return nullptr;
    return reinterpret_cast<void*>(address);
}

void unmap(void* ptr, uint64_t size) { agn_vm_deallocate(taskSelf, reinterpret_cast<uint64_t>(ptr), size); }

int64_t openRead(const char* path) { return openFile(path, kOpenRead, false); }

int64_t openCreate(const char* path) { return openFile(path, kOpenWrite | kOpenCreate, true); }

int64_t closeFd(int fd) {
    uint32_t port = portFor(fd);
    if (port == 0) return -1;
    descriptorPorts[fd] = 0;
    return agn_mach_port_deallocate(taskSelf, port) == 0 ? 0 : -1;
}

int64_t argCount() { return argcValue; }

const char* argAt(int64_t index) {
    if (index < 0 || index >= argcValue) return nullptr;
    return argvValue[index];
}

} // namespace agn::platform
