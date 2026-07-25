---
title: Toolchain
description: CLI flags, backends, memory modes, and packaging.
---

## CLI flags

```
agnostic <source.agn> [options]
  --backend=llvm|nvm|gcc   select codegen backend (default: llvm)
  --mem=arc|manual|orc     select memory management mode (default: arc)
  --target-os=linux|freebsd|windows|hurd  (default: linux, only linux implemented)
  --output=<path>          output executable path
  --version                print version and exit
  --help                   print usage and exit
```

`--llvm`, `--nvm`, and `--gcc` are accepted as shorthand for the matching `--backend=` value.

## Backends

### llvm (default)

Generates native machine code through the LLVM C++ API (`IRBuilder`, `Module`, `TargetMachine`), then links it into a static executable with `cc -nostdlib -static -no-pie -e _start`. The result does not link libc; the standard library and runtime call the Linux kernel through raw syscalls (`src/platform/linux`). This is the only backend with working closures, pointers, structs, `comptime`, and template strings.

### nvm

Generates bytecode for the Novaria Virtual Machine and writes it to `<output>.bin`. Running it needs a Novaria kernel or a separate bytecode interpreter; this repository does not ship one. Not supported under this backend: closures and function values, `&` address-of, template string interpolation, the `++` string operator, and `stdio.ReadInt`/`ReadChar`/`ReadLine` (the Novaria kernel's stdin has no working read path). Each of these is a compile-time error naming the specific construct, not a silent miscompile.

### gcc

Not implemented. Selecting `--backend=gcc` fails with "the gcc/libgccjit backend is not implemented yet".

## Memory modes

`--mem=` only affects the `llvm` backend; `nvm` bytecode has no heap allocation runtime to select a strategy for.

- **arc** (default): reference counting. `agn_rt_retain`/`agn_rt_release` run around bindings that alias an existing value; a fresh construction (a literal, a call result, a struct literal) is not retained again because it already owns its one reference.
- **manual**: `agn_rt_alloc` only. Nothing is freed automatically.
- **orc**: region-based. Every function call opens a region on entry and frees everything allocated in it, in bulk, on every return path, including `main`. This is cheaper than `arc` but has a real limitation: a pointer allocated inside a function and returned to (or stored somewhere reachable from) the caller is unsafe once that function's region has been torn down. There is no escape analysis to catch this; it is the caller's responsibility.

## target-os

Only `linux` has a real platform implementation. `freebsd`, `windows`, and `hurd` are unimplemented stubs; compiling with `--backend=llvm` and any `--target-os=` other than `linux` fails with "only --target-os=linux has a real platform/runtime implementation".

## Building the compiler from source

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

Requires LLVM development files on the CMake search path and a C++20 compiler. See [Getting Started](/en/getting-started/).

## Installing from a package

CI builds `.deb`, `.rpm`, an Arch package, and an Alpine `.apk` on every tag push, and attaches them to the GitHub release. All four install the same layout: the `agnostic` binary under `bin/`, the runtime static libraries under `lib/agnostic/`, and the standard library `.agn` files under `share/agnostic/stdlib/`. The compiler looks for its standard library and runtime libraries relative to its own path first, then falls back to the layout used when running straight out of the build directory, so an installed package and a locally built binary both work without extra configuration.
