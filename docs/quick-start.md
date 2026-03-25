# VEL Quick Start

Get up and running with VEL in 5 minutes.

---

## Step 1 — Build VEL

VEL compiles from source with any C99 compiler. No package manager, no runtime, no dependencies.

```bash
git clone https://github.com/vel-lang/vel
cd vel
make
./vel version   # vel 0.1.0
```

Or build with a single command:

```bash
cc -std=c99 -O2 -o vel src/*.c -lm
```

> **Requirements:** Any C99 compiler (`gcc`, `clang`, `tcc`, `msvc`) + `libc` + `libm`. Nothing else.

---

## Step 2 — Hello World

Create `hello.vel`:

```vel
say "Hello, world!"
```

Run it:

```bash
./vel run hello.vel
# Hello, world!
```

---

## Step 3 — Variables and Functions

```vel
let name = "Alice"
let age  = 28

fn greet(n: Text) -> "Hello, " + n

say greet(name)        // Hello, Alice
say "Age: " + age      // Age: 28
```

`let` declares a mutable variable. `fix` declares an immutable constant. Types are always inferred.

---

## Step 4 — Control Flow

```vel
// if / else
if age >= 18:
    say "Welcome"
else:
    say "Come back later"

// match
fn label(n: Int) -> Text:
    match n:
        1 -> "one"
        2 -> "two"
        _ -> "other"

// for loop
for i in range(1, 6):
    say i

// while loop
let x = 1
while x < 10:
    x = x * 2
say x   // 16
```

---

## Step 5 — Error Handling

VEL uses `Result[T]` instead of exceptions:

```vel
fn divide(a: Float, b: Float) -> Result[Float]:
    if b == 0: -> fail("Division by zero")
    -> ok(a / b)

match divide(10.0, 2.0):
    ok(val)  -> say "Result: " + val   // Result: 5.0
    fail(e)  -> say "Error: " + e

// Propagate errors with ?
fn safeCompute(a: Float, b: Float) -> Result[Float]:
    let v = divide(a, b)?
    -> ok(v * 2.0)
```

---

## Step 6 — Classes

```vel
class Animal:
    name:  Text
    sound: Text
    fn speak() -> name + " says " + sound

class Dog extends Animal:
    breed: Text
    fn fetch() -> name + " fetches!"

let rex = Dog(name: "Rex", sound: "Woof", breed: "Labrador")
say rex.speak()    // Rex says Woof
say rex.fetch()    // Rex fetches!
```

Methods access fields directly — no `self.` prefix needed.

---

## Step 7 — AI Features

```vel
// Route messages by semantic intent
fn handleMessage(msg: Text):
    route msg:
        if feels like "billing"  -> say "-> billing team"
        if feels like "bug"      -> say "-> engineering"
        else                     -> say "-> general support"

handleMessage("My invoice is wrong")     // -> billing team
handleMessage("The app keeps crashing")  // -> engineering
handleMessage("Hello there")             // -> general support
```

---

## Step 8 — Interactive REPL

```bash
./vel repl
```

```
vel> let x = 42
vel> say x * 2
84
vel> fn double(n) -> n * 2
vel> say double(21)
42
vel> .exit
```

---

## Cheat Sheet

| Concept | Syntax |
|---------|--------|
| Mutable variable | `let x = 42` |
| Immutable constant | `fix PI = 3.14` |
| Function | `fn add(a, b) -> a + b` |
| Multi-line function | `fn foo(x):\n    -> x * 2` |
| Lambda | `fn(x) -> x * 2` |
| Class | `class Dog: name: Text; fn bark() -> ...` |
| Inheritance | `class Poodle extends Dog: ...` |
| If / else | `if x > 0: ... else: ...` |
| Match | `match x: 1 -> "one"; _ -> "other"` |
| For loop | `for item in list: ...` |
| While loop | `while condition: ...` |
| Repeat | `repeat 5 times: ...` |
| Async | `let r = wait fetchData()` |
| Error propagation | `let v = riskyCall()?` |
| Print | `say "Hello"` |
| Result | `ok(42)` / `fail("msg")` |
| Option | `some(42)` / `none` |
| AI query | `wait ask data: "plain english query"` |
| AI inference | `wait infer from data: "what to infer"` |
| Intent routing | `route msg: if feels like "billing" -> ...` |

---

## Next Steps

- Read the full [Language Specification](language-spec.md)
- Browse [example programs](../examples/)
- Try the [Cookbook](cookbook.md) for real-world patterns
- Run the test suite: `bash tests/run_tests.sh`

---

*© 2026 VEL Language Foundation — vel-lang.dev*
