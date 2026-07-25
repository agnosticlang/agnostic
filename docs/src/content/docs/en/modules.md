---
title: Modules
description: import, module resolution, and calling functions across modules.
---

## Importing

```agn
import "stdio"
import "math"
```

`import "name"` looks for `name.agn`, searched in this order: next to the file doing the importing, then `stdlib/` under the current working directory, then a `stdlib/` directory next to the compiler binary, then `../share/agnostic/stdlib/` relative to the compiler binary (the layout used by installed packages). The first match wins.

## Calling into a module

Every function called from outside its own file needs the `pub` modifier (see [Syntax](/en/syntax/)); a function without it is invisible to importers and is not even type-checked when the file that defines it is loaded as a module. Call an imported function with `module.FunctionName(...)`:

```agn
package main

import "math"
import "stdio"

func main() {
    stdio.Println(math.Max(3, 7))
}
```

## Calling within the same module

A function defined in a module file can call a sibling function in that same file by its bare name, with no module prefix:

```agn
// inside string.agn
pub fn is_empty(s string) int {
    if len(s) == 0 {
        return 1
    }
    return 0
}
```

`len` here resolves to `string.len` because both functions are declared in the same file. This resolution is based on the file the call is written in, not on `pub`: an unexported helper cannot be called this way either, because it was never compiled in the first place.

## No circular safety net

There is no cycle detection for imports beyond not re-parsing a path already loaded in the current compilation; keep the module graph a DAG in practice.

## package main

`package main` is not itself a module other code imports; it is the entry point compiled directly, and it does not need `pub` on any of its functions.
