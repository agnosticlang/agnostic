---
title: Functions and Closures
description: Function values, closures, and capture semantics.
---

## Function values

A function literal has the same parameter and return syntax as a named function, with no name:

```agn
var double func(i64) -> i64 = func(x i64) -> i64 {
    return x * 2
}
```

A named top-level function can also be used as a value, by referring to it by name with no call parentheses:

```agn
func linuxWrite(fd i64, buf i64, len i64) i64 {
    return 1
}

var writer func(i64, i64, i64) -> i64 = linuxWrite
```

## Closures

A function literal captures the variables it references from its enclosing scope, by reference:

```agn
func makeCounter() func() -> i64 {
    var count i64 = 0
    var next func() -> i64 = func() -> i64 {
        count = count + 1
        return count
    }
    return next
}

func main() {
    var counter func() -> i64 = makeCounter()
    stdio.Println(counter())   // 1
    stdio.Println(counter())   // 2
    stdio.Println(counter())   // 3
}
```

`count` outlives `makeCounter`'s own call frame because it escapes into the returned closure; the compiler detects this and heap-allocates any local that is captured by an escaping closure literal instead of putting it on the stack.

## Calling a function value

```agn
func apply(f func(i64) -> i64, x i64) i64 {
    return f(x)
}

apply(double, 21)
```

## Backend support

Function values and closures only work under `--backend=llvm`. `--backend=nvm` rejects any function literal or bare function-name-as-value at compile time: the Novaria Virtual Machine's `CALL` instruction only takes a compile-time bytecode address, there is no indirect call instruction to call through a runtime function pointer.
