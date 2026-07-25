---
title: Comptime
description: Compile-time conditionals on target OS, architecture, and memory mode.
---

A `comptime` block runs its condition at compile time and keeps only the branch that matches, deleting the other branch from the program entirely before code generation ever sees it. It is not a general compile-time evaluator; it only exists to pick code paths based on three built-in constants.

```agn
struct Platform {
    write func(i64, i64, i64) -> i64
}

func linuxWrite(fd i64, buf i64, len i64) i64 {
    return 1
}

func stubWrite(fd i64, buf i64, len i64) i64 {
    return 0
}

func main() {
    var platform Platform
    comptime {
        if TARGET_OS == "linux" {
            platform.write = linuxWrite
        } else {
            platform.write = stubWrite
        }
    }
    stdio.Println(platform.write(1, 0, 4))
}
```

## Available constants

- `TARGET_OS`: the value of `--target-os=` (`linux`, `freebsd`, `windows`, or `hurd`).
- `TARGET_ARCH`: the target architecture (`x86_64`).
- `MEM_MODE`: the value of `--mem=` (`arc`, `manual`, or `orc`).

## Condition grammar

A `comptime if` condition is one of these constants compared to a string literal with `==` or `!=`, combined with `&&`, `||`, and `!`:

```agn
comptime {
    if TARGET_OS == "linux" && MEM_MODE == "arc" {
        // ...
    }
}
```

Anything else in the condition (a variable, a function call, a numeric comparison) fails to evaluate at compile time and is a compile error. There is no comptime loop and no comptime function evaluation; the body of each branch is ordinary runtime code, only the choice of which branch survives happens at compile time.
