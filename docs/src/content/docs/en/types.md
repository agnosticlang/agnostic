---
title: Types
description: The Agnostic type system.
---

## Scalars

| Type | Meaning |
|---|---|
| `i64` | 64-bit signed integer |
| `i32` | 32-bit signed integer |
| `i8` | 8-bit signed integer |
| `u64` | 64-bit unsigned integer |
| `u32` | 32-bit unsigned integer |
| `u8` | 8-bit unsigned integer |
| `f64` | 64-bit floating point |
| `bool` | boolean |
| `string` | string |
| `int` | alias for `i64` |
| `float` | alias for `f64` |

A literal with a `.` (`3.14`) is `f64`; a literal without one (`3`) is `i64`. Mixing `f64` and an
integer in arithmetic (`3.14 + 2`) implicitly converts the integer operand to `f64` and the
expression's type is `f64`; `%` and the bitwise/shift operators do not accept `f64` operands.
`f64` is only supported under `--backend=llvm` and `--backend=gcc` — `--backend=nvm` rejects any
use of `f64` at compile time, since its bytecode stack machine is 32-bit-integer-only throughout
and there is no in-repo interpreter to verify a float encoding against.

There is no `char` type; a byte read from a string index or `stdio.ReadChar` is an `int`.

## Type inference

`var x = expr` infers the type of `x` from `expr`. `var x int = expr` declares the type explicitly and checks `expr` against it. A declaration with no initializer (`var x int`) gets the type's zero value.

## Pointers

`&x` takes the address of `x`. A pointer type is written `*T`, and `*p` reads or writes the value it points to:

```agn
func set(p *int, v int) {
    *p = v
}

func main() {
    var x int = 5
    var p *int = &x
    *p = 99
    set(&x, 7)
}
```

`*[N]T` converts implicitly to `*T`, a pointer to the first element. This is how a byte array is passed as a buffer:

```agn
var buf [64]u8
var n Option<int> = os.ReadFd(fd, &buf, 64)
```

Pointers are supported under `--backend=llvm` and `--backend=gcc`. `--backend=nvm` rejects `&` at compile time: ordinary local variables in the Novaria Virtual Machine's bytecode are frame-relative stack slots, not addressable memory.

## Arrays

Fixed size, declared with the size and element type before the variable is usable:

```agn
var arr [8]int
arr[0] = 5
var first int = arr[0]
```

Array size is a compile-time integer literal. There is no array literal syntax and no slicing; every element is set individually or in a loop.

## Structs

See [Structs](/en/structs/).

## Function types

Written `func(T1, T2) -> R`, or `func() -> R` with no parameters, or `func(T1)` with an inferred `void` return. See [Functions and Closures](/en/functions/).

## Casts

`expr as T` converts a value explicitly:

```agn
var big int = 300
var low u8 = big as u8
var half = 7 as f64 / 2.0
var flag int = true as int
var addr u64 = &big as u64
```

| From | To | Result |
|---|---|---|
| integer | integer | truncated to the target width, or extended: sign-extended from a signed type, zero-extended from an unsigned one |
| integer | `f64` | the nearest `f64` value |
| `f64` | integer | truncated toward zero; the value must fit in the target type |
| `bool` | integer | `0` or `1` |
| pointer or `string` | pointer or `string` | the same address with the new type |
| pointer | `i64` or `u64` | the address |
| `i64` or `u64` | pointer | the address |

Any other cast is a compile error. There is no cast from a number to `bool`; compare instead (`x != 0`).

`as` binds tighter than the binary operators and looser than the unary ones: `-x as u8` is `(-x) as u8`, and `a + b as i64` is `a + (b as i64)`. Under `--backend=nvm`, casts to `f64` and to pointer types are compile errors.

## Type checking rules

A value converts implicitly only in these cases:

- between numeric types (`i64`, `i32`, `i8`, `u64`, `u32`, `u8`, `f64`); the emitted code truncates or extends the value to the target width;
- from `*[N]T` to `*T`;
- from `*u8`, `*i8`, `*[N]u8`, or `*[N]i8` to `string`.

Any other conversion needs an explicit [cast](#casts) or is a compile error. `bool` and numbers do not convert into each other, and pointers do not convert to or from numbers. The conditions of `if` and `for` and the operands of `!`, `&&`, and `||` must be `bool`. `==` and `!=` need operands of compatible types, and `<`, `<=`, `>`, `>=` need numeric operands. The error message suggests the cast or comparison to write:

```
error: type mismatch in variable 'n': declared as i64, initialized with bool (convert explicitly with 'as i64')
error: condition must be bool, got i64 (compare instead, e.g. 'x != 0')
```

## Generics and pattern matching

Generic *structs*, resolved at compile time by monomorphization, exist — see [Generic
structs](/en/structs/#generic-structs). Generic *functions* exist too, via `comptime` parameters instead
of `<...>` syntax — see [Generic functions](/en/comptime/#generic-functions). There is no `match`/`switch`
expression; branching is `if`/`else` only (see [Control Flow](/en/control-flow/)).
