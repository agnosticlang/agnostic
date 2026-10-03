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

A literal with a `.` (`3.14`) is `f64`; a literal without one (`3`) is `i64`. An integer literal also
takes the type it is used as when its value fits: `var b u8 = 200` and `3.14 + 2` compile,
`var b u8 = 256` does not. `%` and the bitwise/shift operators do not accept `f64` operands.

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

`as` binds tighter than the binary operators and looser than the unary ones: `-x as u8` is `(-x) as u8`, and `a + b as i64` is `a + (b as i64)`.

## Type checking rules

A value converts implicitly only in these cases:

- from a numeric type to one that holds every value of it: `i8` to `i32` to `i64`, `u8` to `u32` to `u64`, an unsigned type to a wider signed one (`u8` to `i32`, `u32` to `i64`), and `i8`, `i32`, `u8`, `u32` to `f64`;
- from an integer literal, or a negated one, to any numeric type that holds its value;
- from `*[N]T` to `*T`;
- from `*u8`, `*i8`, `*[N]u8`, or `*[N]i8` to `string`.

Any other conversion needs an explicit [cast](#casts) or is a compile error. Narrowing (`i64` to `i8`), changing signedness (`i32` to `u32`), `f64` to an integer, and `i64` or `u64` to `f64` all lose values for some inputs, so they are written with `as`. `bool` and numbers do not convert into each other, and pointers do not convert to or from numbers. The conditions of `if` and `for` and the operands of `!`, `&&`, and `||` must be `bool`. `==` and `!=` need operands of compatible types, and `<`, `<=`, `>`, `>=` need numeric operands.

The two operands of an arithmetic, bitwise, or comparison operator are brought to one type first. An integer literal takes the type of the other operand (`x + 1` with `x i8` is `i8`); otherwise the operand of the smaller type converts to the other one by the rules above (`i8 + i64` is `i64`). Operands that neither rule unifies, such as `i32` and `u32` or `f64` and `i64`, are a compile error. The right operand of `<<` and `>>` is a shift count and can be any integer type.

The error message suggests the cast or comparison to write:

```
error: type mismatch in variable 'n': declared as i64, initialized with bool (convert explicitly with 'as i64')
error: condition must be bool, got i64 (compare instead, e.g. 'x != 0')
error: type mismatch in variable 'b': declared as u8, initialized with i64 (constant 256 does not fit in u8)
error: mismatched operand types f64 and i64 (convert one side explicitly with 'as')
```

## Generics and pattern matching

Generic *structs*, resolved at compile time by monomorphization, exist — see [Generic
structs](/en/structs/#generic-structs). Generic *functions* exist too, via `comptime` parameters instead
of `<...>` syntax — see [Generic functions](/en/comptime/#generic-functions). There is no `match`/`switch`
expression; branching is `if`/`else` only (see [Control Flow](/en/control-flow/)).
