---
title: Structs
description: Fields, methods, and struct literals.
---

## Declaration

```agn
struct Vector2 {
    x i64,
    y i64
}
```

Fields are comma-separated `name type` pairs. There is no inheritance and no vtables: a struct cannot extend another struct, and methods are resolved statically at compile time, not through a dispatch table.

## Struct literals

```agn
var a Vector2 = Vector2{x: 3, y: 4}
```

Every field must be named in the literal (`field: value`); there is no positional form.

## Methods

A method is a function with a receiver in parentheses before the name:

```agn
func (v Vector2) Length2() i64 {
    return v.x * v.x + v.y * v.y
}

func (v Vector2) Add(other Vector2) Vector2 {
    return Vector2{x: v.x + other.x, y: v.y + other.y}
}
```

Call it with `.`:

```agn
var a Vector2 = Vector2{x: 3, y: 4}
stdio.Println(a.Length2())   // 25
```

The receiver is passed by value like any other parameter; a method that needs to mutate the caller's struct should be called on a variable, and field assignment goes through that variable directly (`a.x = 10`), not through the method's own receiver copy.

## Function-typed fields

A struct field can hold a function value:

```agn
struct Platform {
    write func(i64, i64, i64) -> i64
}

var platform Platform
platform.write = linuxWrite
stdio.Println(platform.write(1, 0, 4))
```

This only works under `--backend=llvm`, for the same reason plain closures do not work under `--backend=nvm`: see [Functions and Closures](/en/functions/).

## Field access and assignment

```agn
var a Vector2 = Vector2{x: 3, y: 4}
a.x = 10
stdio.Println(a.x)
```
