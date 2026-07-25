---
title: Control Flow
description: if, for, break, and continue.
---

## if / else

```agn
if x > 0 {
    stdio.Println(1)
} else {
    stdio.Println(0)
}
```

Braces are required. There is no `else if`; an `else` branch that starts with another `if` must nest it in its own block:

```agn
if x % 15 == 0 {
    stdio.PrintlnStr("FizzBuzz")
} else {
    if x % 3 == 0 {
        stdio.PrintlnStr("Fizz")
    } else {
        stdio.Println(x)
    }
}
```

## for

`for` covers every loop form. With a condition, it behaves like a `while` loop:

```agn
var i int = 0
for i < 10 {
    stdio.Println(i)
    i = i + 1
}
```

With no condition, it loops forever until a `break`:

```agn
for {
    if done() {
        break
    }
}
```

`while` and `loop` are accepted as aliases for `for`; the grammar is the same either way, condition or none. There is no C-style three-part `for (init; cond; step)` form and no `for x in range`.

## break and continue

`break` exits the nearest enclosing loop. `continue` skips to the next iteration's condition check. Both are rejected at type-check time outside a loop, and a `break` or `continue` written inside a closure body only sees loops in that closure's own body, not loops in the function the closure is defined inside.

```agn
var found int = 0
var n int = 2
for found < 10 {
    if math.IsPrime(n) == 0 {
        n = n + 1
        continue
    }
    stdio.Println(n)
    found = found + 1
    n = n + 1
}
```

## return

`return` with no value exits a function that returns nothing. `return expr` exits with a value; the function's declared return type and `expr`'s type must be compatible. A function falling off the end of its body without an explicit `return` returns the return type's zero value (or, for `main`, exits with status 0).
