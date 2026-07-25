---
title: Начало работы
description: Сборка компилятора Agnostic и первая программа.
---

Компилятор — это проект на CMake, написанный на C++20. Для сборки нужен LLVM с `LLVMConfig.cmake` в путях поиска CMake (в разработке использовались версии LLVM с 18 по 22) и компилятор с поддержкой C++20, например GCC 12+ или Clang 15+.

## Сборка компилятора

```sh
cmake -S . -B build
cmake --build build -j$(nproc)
```

В результате получается `build/src/cli/agnostic`. Проверить, что всё работает:

```sh
./build/src/cli/agnostic --version
```

## Программа

```agn
package main

import "stdio"

func main() {
    stdio.PrintlnStr("Hello, World!")
}
```

Сохраните это как `hello.agn`.

## Компиляция и запуск

```sh
./build/src/cli/agnostic hello.agn --backend=llvm --output=hello
./hello
```

`--backend=llvm` (по умолчанию) собирает статический Linux-исполняемый файл без libc. Внутри компилятор генерирует объектный файл и линкует его командой `cc -nostdlib -static -no-pie -e _start`, поэтому компилятор C должен быть доступен в `PATH` во время сборки, хотя итоговый бинарник от libc не зависит.

`--backend=nvm` вместо нативного исполняемого файла создаёт файл `.bin` с байткодом Novaria Virtual Machine. Для его запуска нужно ядро Novaria или интерпретатор байткода — ни то, ни другое в этом репозитории не поставляется.

Дальше — [Синтаксис](/ru/syntax/) про сам язык, или [Инструментарий](/ru/toolchain/) про полный список флагов компилятора.
