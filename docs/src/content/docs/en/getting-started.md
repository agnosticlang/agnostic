---
title: Getting Started
description: Build the Agnostic compiler and compile your first program.
---

The compiler is a CMake project written in C++20. Building it requires LLVM with `LLVMConfig.cmake` on the CMake search path (development has used LLVM 18 through 22) and a C++20 compiler such as GCC 12+ or Clang 15+.

## Build the compiler

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

This produces `build/src/cli/agnostic`. Confirm it works:

```sh
./build/src/cli/agnostic --version
```

## Write a program

```agn
package main

import "stdio"

func main() {
    stdio.PrintlnStr("Hello, World!")
}
```

Save this as `hello.agn`.

## Compile and run

```sh
./build/src/cli/agnostic hello.agn --backend=llvm --output=hello
./hello
```

`--backend=llvm` (the default) produces a static, libc-free Linux executable. Internally the compiler emits an object file and links it with `cc -nostdlib -static -no-pie -e _start`, so a C compiler needs to be on `PATH` at compile time even though the resulting binary does not depend on libc at runtime.

`--backend=nvm` produces a `.bin` file of Novaria Virtual Machine bytecode instead of a native executable. Running it needs a Novaria kernel or a bytecode interpreter, neither of which ships in this repository.

Continue with [Syntax](/en/syntax/) for the language itself, or [Toolchain](/en/toolchain/) for the full list of compiler flags.
