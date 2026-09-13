<div align="center" style="display:grid;place-items:center;">
<h1>The Agnostic Programming Language</h1>
Write Once, Run Anywhere.
</div>
<br>

<p align="center">
  <img src="https://img.shields.io/github/actions/workflow/status/agnosticlang/agnostic/ci.yml?branch=main&style=flat-for-the-badge&logo=github" alt="Build Status">
  <img src="https://img.shields.io/github/license/agnosticlang/agnostic?style=flat-for-the-badge&color=blue" alt="License">
  <img src="https://img.shields.io/github/stars/agnosticlang/agnostic?style=flat-for-the-badge&color=gold" alt="Stars">
  <img src="https://img.shields.io/github/issues/agnosticlang/agnostic?style=flat-for-the-badge&color=red" alt="Issues">
</p>
</div>
<br>

This repository contains the Agnostic compiler, standard library and tools.

### Building

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

Requires LLVM (found via `find_package(LLVM CONFIG REQUIRED)`). The
resulting `agnostic` binary supports `--backend=llvm|nvm|gcc`,
`--mem=arc|manual|orc`, and `--target-os=`.

### Examples

Examples can be found in the `examples/`[*](https://github.com/agnosticlang/agnostic/tree/main/examples) directory.

The original authors are [Zennix](https://github.com/z3nnix/) and [Noxzion](https://github.com/noxzion/)

### License

The compiler (`src/`, `include/`, `old/`, build/packaging scripts) is licensed under
[GPL-3.0-or-later](LICENSES/GPL-3.0-or-later.txt), because the `gcc` backend links against `libgccjit`
(GPLv3). The standard library, examples, and test fixtures (`stdlib/`, `examples/`,
`scripts/testdata/`) are licensed under [Apache-2.0](LICENSES/Apache-2.0.txt) — programs you compile
with Agnostic are not affected by the compiler's license. See each file's `SPDX-License-Identifier`
header for which license applies to it.
