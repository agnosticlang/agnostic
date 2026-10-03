---
title: Standard Library
description: result, stdio, math, string, and os.
---

Every function in these modules is a compiler intrinsic: the body written in the `.agn` source exists only so the file type-checks and so a human reading it can see the signature. The actual behavior is generated directly by the compiler backend, not by running that body.

## result

`stdlib/result.agn` declares `Result<T,E>` (`ok bool`, `value T`, `err E`) and `Option<T>` (`some bool`, `value T`). Unlike every other module here, these two generic structs are always in scope — no `import "result"` needed — and their fields are real data, not a compiler-generated body; the compiler parses this file directly and merges its declarations into every compiled program. See [Generic structs](/en/structs/#generic-structs) for how instantiation (`Result<int,string>`) and monomorphization work.

## stdio

| Function | Signature | Notes |
|---|---|---|
| `Print` | `(value int)` | writes a decimal integer, no newline |
| `Println` | `(value int)` | writes a decimal integer and a newline |
| `PrintStr` | `(text string)` | writes a string, no newline |
| `PrintlnStr` | `(text string)` | writes a string and a newline |
| `PrintFloat` | `(value float)` | writes an `f64` with six digits after the point, no newline |
| `PrintlnFloat` | `(value float)` | writes an `f64` with six digits after the point and a newline |
| `PrintBool` | `(value bool)` | writes `true` or `false`, no newline |
| `PrintlnBool` | `(value bool)` | writes `true` or `false` and a newline |
| `PrintChar` | `(ch int)` | writes one byte |
| `ReadInt` | `() int` | reads a decimal integer from stdin |
| `ReadChar` | `() int` | reads one byte from stdin |
| `ReadLine` | `(buffer *u8, maxlen int) int` | reads a line into `buffer`, returns the byte count |
| `Flush` | `()` | no-op; output is unbuffered |

`Print` and `Println` only accept integers; passing an `f64`, a `string`, or a `bool` is a type error, use `PrintFloat`/`PrintlnFloat`, `PrintStr`/`PrintlnStr`, or `PrintBool`/`PrintlnBool`.

## math

All functions take `int`; most return `int`, except `IsEven`/`IsOdd`/`IsPrime`, which return `bool`.

| Function | Signature | Notes |
|---|---|---|
| `Max` | `(a int, b int) int` | |
| `Min` | `(a int, b int) int` | |
| `Pow` | `(base int, exp int) int` | |
| `Sqrt` | `(n int) int` | integer square root, Newton's method, positive input only |
| `GCD` | `(a int, b int) int` | Euclidean algorithm, positive input only |
| `LCM` | `(a int, b int) int` | positive input only |
| `Fact` | `(n int) int` | factorial |
| `IsEven` | `(n int) bool` | |
| `IsOdd` | `(n int) bool` | |
| `Sign` | `(x int) int` | 1 if `x > 0`, otherwise 0; negative input is not distinguished from zero |
| `Clamp` | `(value int, min int, max int) int` | |
| `SumRange` | `(n int) int` | sum of `1..n` |
| `IsPrime` | `(n int) bool` | trial division |
| `Fib` | `(n int) int` | n-th Fibonacci number, iterative |

## string

| Function | Signature | Notes |
|---|---|---|
| `len` | `(s string) int` | byte length |
| `compare` | `(s1 string, s2 string) int` | 0 if equal, -1 if `s1 < s2`, 1 if `s1 > s2` |
| `concat` | `(s1 string, s2 string) string` | |
| `is_empty` | `(s string) bool` | |
| `indexOf` | `(s string, sub string) Option<int>` | index of the first occurrence of `sub`; `some=false` if not found |
| `contains` | `(s string, sub string) bool` | |
| `startsWith` | `(s string, prefix string) bool` | |
| `endsWith` | `(s string, suffix string) bool` | |
| `charAt` | `(s string, index int) Option<int>` | byte code at `index`; `some=false` if out of bounds |
| `substr` | `(s string, start int, len int) string` | clamped to the string's bounds |
| `toUpper` | `(s string) string` | ASCII only |
| `toLower` | `(s string) string` | ASCII only |

`++` and `$(...)` template string interpolation (see [Syntax](/en/syntax/)) cover the same ground as `concat` for simple cases.

## os

File descriptors and command-line arguments.

| Function | Signature | Notes |
|---|---|---|
| `ArgCount` | `() int` | number of command-line arguments, including argv[0] (the program path) |
| `Arg` | `(index int) string` | argument at `index` |
| `OpenRead` | `(path string) Option<int>` | value is the file descriptor; `some=false` on error |
| `OpenCreate` | `(path string) Option<int>` | create/truncate for writing; value is the file descriptor, `some=false` on error |
| `Close` | `(fd int) int` | |
| `ReadFd` | `(fd int, buffer *u8, maxlen int) Option<int>` | reads into `buffer`, like `stdio.ReadLine`; value is bytes read, `some=false` on error |
| `WriteFd` | `(fd int, data string) Option<int>` | value is bytes written, `some=false` on error |
| `Exit` | `(code int)` | terminates the process immediately, bypassing any remaining code in the caller |

For a `[N]u8` array `buf`, pass `&buf` as the `buffer` of `ReadFd` and `stdio.ReadLine`. See [Pointers](/en/types/#pointers).

`OpenRead`, `OpenCreate`, `ReadFd`, and `WriteFd` return `Option<int>` (see [Generic structs](/en/structs/#generic-structs)) instead of a raw sentinel: `some` is `true` on success and `false` on failure, and `value` holds the file descriptor or byte count only when `some` is `true` — check `some` before trusting `value`.
