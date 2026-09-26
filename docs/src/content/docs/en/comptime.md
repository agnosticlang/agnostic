---
title: Comptime
description: Compile-time code execution, conditional compilation, and generic functions.
---

A `comptime` block is interpreted at compile time by a tree-walking evaluator before code generation ever
runs. It can compute arbitrary values — arithmetic, loops, calls to other top-level functions — and the
results are spliced into the program as literals. Anything a `comptime` block cannot evaluate (an `if`
condition aside) is simply left as ordinary runtime code, so existing conditional-compilation code keeps
working unchanged.

## Compile-time evaluation

```agn
func fib(n int) int {
    if n < 2 {
        return n
    }
    return fib(n - 1) + fib(n - 2)
}

func main() {
    comptime {
        var fibResult int = fib(10)
    }
    stdio.Println(fibResult)   // 55, computed at compile time

    comptime {
        var sum int = 0
        var i int = 0
        for i < 5 {
            sum = sum + i
            i = i + 1
        }
        var total int = sum
    }
    stdio.Println(total)   // 10
}
```

`var`, assignment, `for`, plain expression statements, and nested `comptime` blocks are interpreted.
A value computed this way (`fibResult`, `total`) is bound for the rest of the enclosing function — like
every other variable in Agnostic, `comptime` bindings are function-scoped, not block-scoped — and every
later reference to that name is replaced with the literal value before codegen sees it. No backend needs
to know a value was ever comptime-computed; it only ever sees a plain literal.

A statement the evaluator cannot handle (a call into a function with a receiver, a struct literal,
anything referencing runtime-only state) is left exactly as it appears and typechecked normally as
ordinary runtime code — there is no error for this, it is the intended fallback that keeps constructs like
the platform-dispatch pattern below working.

A fixed budget of 10,000,000 evaluated statements guards against an infinite `comptime` loop; exceeding it
is a compile error.

## Conditional compilation

`comptime if` still works exactly as before: the condition is evaluated at compile time, and only the
taken branch survives into the compiled program — the other branch is deleted before codegen. Unlike a
plain `comptime` statement, a condition that cannot be evaluated at compile time *is* a compile error.

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

The condition can be any compile-time-evaluable expression, not just a comparison against the built-in
constants — arithmetic, function calls, and combinations with `&&`/`||`/`!` all work.

### Available constants

- `TARGET_OS`: the value of `--target-os=` (`linux`, `freebsd`, `windows`, or `hurd`).
- `TARGET_ARCH`: the target architecture (`x86_64`).
- `MEM_MODE`: the value of `--mem=` (`arc`, `manual`, or `orc`).

## Generic functions

A function parameter marked `comptime` is resolved entirely at compile time and does not exist in the
function's runtime signature. Declaring one as `type` makes it a type parameter; declaring it as any other
type (`int`, `bool`, `string`, `f64`) makes it a compile-time value parameter:

```agn
func max(comptime T type, a T, b T) T {
    if a > b {
        return a
    }
    return b
}

func addN(comptime n int, x int) int {
    return x + n
}

func main() {
    stdio.Println(max(int, 3, 7))     // 7 — T = int
    stdio.Println(addN(5, 10))        // 15 — n = 5
}
```

The type or value is passed like any other positional argument — `max(int, 3, 7)`, not
`max<int>(3, 7)` — and can be any compile-time-constant expression for a value parameter (`addN(2 + 3, x)`
works the same as `addN(5, x)`). Every distinct combination of comptime arguments used in the program
compiles to its own concrete function behind the scenes (monomorphization), the same way [generic
structs](/en/structs/#generic-structs) work. A generic function can call another generic function using
its own comptime parameters, and the outer values are forwarded correctly:

```agn
func identity(comptime T type, x T) T {
    return x
}

func scaleAndAdd(comptime T type, comptime factor int, x T) T {
    return identity(T, x) * factor
}
```

Not supported yet: comptime parameters on methods (functions with a receiver), using a comptime value in a
type position (a comptime-sized array), and inferring the type argument from a regular argument instead of
passing it explicitly.
