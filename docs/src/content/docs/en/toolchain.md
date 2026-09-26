---
title: Toolchain
description: CLI flags, backends, memory modes, and packaging.
---

## CLI flags

```
agnostic <source.agn> [options]
  --backend=llvm|gcc       select codegen backend (default: llvm)
  --mem=arc|manual|orc     select memory management mode (default: arc)
  --target-os=linux|freebsd|windows|hurd  (default: linux)
  --output=<path>          output executable path
  --version                print version and exit
  --help                   print usage and exit
```

`--llvm` and `--gcc` are accepted as shorthand for the matching `--backend=` value.

## Backends

### llvm (default)

Generates native machine code through the LLVM C++ API (`IRBuilder`, `Module`, `TargetMachine`), then links it into a static executable with `cc -nostdlib -static -no-pie -e _start`. The result does not link libc; the standard library and runtime call the Linux kernel through raw syscalls (`src/platform/linux`).

### gcc

Generates native machine code through libgccjit, GCC's embeddable code generation library, and links it the same way the `llvm` backend does. It has the same feature set as `llvm` (closures, structs, pointers, `comptime`, all three `--mem=` modes) and shares the same runtime static libraries and libc-free linking. Building the compiler with this backend needs `libgccjit.h` and `libgccjit.so` on the system (Arch Linux: package `libgccjit`; Fedora: `libgccjit-devel`; Debian/Ubuntu: `libgccjit-dev`).

## Memory modes

- **arc** (default): reference counting. `agn_rt_retain`/`agn_rt_release` run around bindings that alias an existing value; a fresh construction (a literal, a call result, a struct literal) is not retained again because it already owns its one reference.
- **manual**: `agn_rt_alloc` only. Nothing is freed automatically.
- **orc**: region-based. Every function call opens a region on entry and frees everything allocated in it, in bulk, on every return path, including `main`. This is cheaper than `arc` but has a real limitation: a pointer allocated inside a function and returned to (or stored somewhere reachable from) the caller is unsafe once that function's region has been torn down. There is no escape analysis to catch this; it is the caller's responsibility.

## target-os

All four targets have real platform implementations. The `freebsd` target links raw amd64 syscalls directly (no libc) and marks the resulting static ELF as a FreeBSD binary with the FreeBSD ABI note (`.note.tag`, `NT_FREEBSD_ABI_TAG`, OS version 14.0), the same note FreeBSD's own startup files add. The note comes from the FreeBSD startup code, so it works with any linker and both backends. Verified by running compiled binaries from both the `llvm` and `gcc` backends (including closures and structs, which exercise the heap allocator) on a real FreeBSD 15.1 VM.

The `windows` target produces a PE32+ console executable, `<output>.exe`, for x86-64, the same way Go does it: the program calls kernel32.dll directly, with no C runtime, no MSVC or MinGW libraries, and no raw NT system calls, whose numbers change between Windows builds. The runtime imports a fixed list of kernel32 functions (`src/platform/windows/kernel32.def`), and the compiler build turns that list into an import library with `llvm-dlltool`. Command-line arguments come from `GetCommandLineW`, are split with the standard Windows quoting rules, and reach the program as UTF-8. The target needs `--backend=llvm`, because libgccjit only generates code for the host, and `lld-link` on `PATH` to link. Verified by running the test programs under Wine.

The `hurd` target produces a static ELF executable for x86-64 GNU/Hurd without glibc. The program talks to GNU Mach and the Hurd servers directly: Mach traps for `mach_msg`, `vm_allocate`, `vm_deallocate`, `mach_port_deallocate`, and `task_terminate`, and MIG messages that the runtime encodes itself for `exec_startup_get_info` (arguments, file descriptors, and initial ports), `io_read`, `io_write`, `dir_lookup`, `file_set_size`, and `proc_mark_exit`. File name lookup follows the retries a server asks for, including absolute symbolic links; a lookup that needs reauthentication or another magical name fails. The program registers no message port, so it receives no signals. This target has not been run on a Hurd system yet: the message layouts come from the GNU Mach, MIG, and Hurd sources, and the test suite only checks that programs compile and link.

## Diagnostics

Type errors report a real `file:line:column`, a colored pointer into the source line, and, where the compiler finds a close match, a suggestion:

```
error: variable 'countr' not declared (did you mean 'counter'?) (in main)
  --> hello.agn:6:5
```

Suggestions are Levenshtein-distance based and only appear when a close-enough candidate exists: undeclared variables (checked against locals and top-level functions in scope), unknown struct fields, and unknown struct names. For `object.member(...)`, the compiler first checks whether `object` itself is a known module, struct-typed variable, or import (`stdi.Println` suggests `stdio`); if `object` resolves but `member` does not, it suggests among that struct's or module's own members, including struct fields holding a function value alongside actual methods (`p.writ(...)` suggests the field `write`).

## Building the compiler from source

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

Requires LLVM development files on the CMake search path and a C++20 compiler. See [Getting Started](/en/getting-started/). The `windows` target also needs `clang`, `llvm-lib`, and `llvm-dlltool` at build time; if one of them is missing, CMake disables the target and says so.

The build bootstraps itself. It first links `agnostic_stage0`, a compiler that uses the C++ lexer in `old/lexer`, compiles the self-hosted lexer `src/lexer/lexer.agn` with it, and then links the final `agnostic` with that lexer. `-DAGNOSTIC_BOOTSTRAP_COMPILER=<path>` compiles `lexer.agn` with an existing compiler instead of building stage 0. `-DAGNOSTIC_BOOTSTRAP=ON` builds only the compiler with the C++ lexer, as `agnostic`.

## Installing from a package

CI builds `.deb`, `.rpm`, an Arch package, and an Alpine `.apk` on every tag push, and attaches them to the GitHub release. All four install the same layout: the `agnostic` binary under `bin/`, the runtime static libraries under `lib/agnostic/`, and the standard library `.agn` files under `share/agnostic/stdlib/`. The compiler looks for its standard library and runtime libraries relative to its own path first, then falls back to the layout used when running straight out of the build directory, so an installed package and a locally built binary both work without extra configuration.
