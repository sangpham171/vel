# VEL Language Specification

> *From Latin velle — to will, to intend.*
> Express intent. The language figures out the rest.

**Version:** 0.1.0 · **License:** MIT · [vel-lang.dev](https://vel-lang.dev)

---

## Table of Contents

1. [Overview](#1-overview)
2. [Syntax Reference](#2-syntax-reference)
3. [Type System](#3-type-system)
4. [Control Flow](#4-control-flow)
5. [Classes and Objects](#5-classes-and-objects)
6. [Async and Concurrency](#6-async-and-concurrency)
7. [Error Handling](#7-error-handling)
8. [AI-Native Features](#8-ai-native-features)
9. [Standard Library](#9-standard-library)
10. [Project Structure](#10-project-structure)
11. [CLI Reference](#11-cli-reference)

---

## 1. Overview

VEL is a programming language built around a single principle: **code should express what you want, not the mechanical steps to achieve it.**

When you write a query in plain language, route a message by intent, or let the type system infer everything it can, you are expressing *volition* — not procedure. The compiler and runtime handle the rest.

### Design Principles

| Principle | Description |
|-----------|-------------|
| **Volitional** | Code expresses intent — the runtime resolves how |
| **Zero ceremony** | No boilerplate, no mandatory annotations, no semicolons |
| **AI-native** | `ask`, `infer`, `feels like` are first-class keywords |
| **Memory-safe** | Automatic ownership — no leaks, no manual management |
| **Concurrent by default** | Every I/O call is async; `parallel:` is one keyword |
| **Deploy anywhere** | Native binary, WebAssembly, or interpreted |

---

## 2. Syntax Reference

### 2.1 Hello World

```vel
say "Hello, world!"
```

No imports, no main function, no semicolons required for simple programs.

### 2.2 Variables

Declare with `let` (mutable) or `fix` (immutable). Types are always inferred.

```vel
let name   = "Alice"
let age    = 30
fix PI     = 3.14159
let score: Float = 98.5      // explicit annotation when needed
```

> **`fix` vs `let`** — `fix` is the immutable declaration. Once assigned, it cannot be reassigned. Use `fix` for constants and configuration values.

### 2.3 Functions

```vel
// One-liner
fn greet(name: Text) -> "Hello, " + name

// Multi-line
fn discount(price: Float, rate: Float) -> Float:
    let saving = price * rate / 100
    -> price - saving

// Anonymous function (lambda)
let double = fn(n: Int) -> n * 2
say double(21)   // 42
```

The `->` operator returns a value. Every function is an expression.

### 2.4 Higher-Order Functions

```vel
fn applyTwice(f: Fn, x: Int) -> f(f(x))

let addFive = fn(x) -> x + 5
say applyTwice(addFive, 10)   // 20

// Functional pipelines
let result = [1, 2, 3, 4, 5]
    .filter(fn(n) -> n > 2)
    .map(fn(n) -> n * 10)
    .reduce(fn(acc, n) -> acc + n, 0)
```

---

## 3. Type System

VEL has a clean, minimal type system. All types are inferred unless you want to be explicit.

| Type | Example | Notes |
|------|---------|-------|
| `Text` | `"Hello"` | Unicode string, immutable |
| `Int` | `42` | 64-bit integer |
| `Float` | `3.14` | 64-bit float |
| `Bool` | `true / false` | Boolean |
| `List[T]` | `[1, 2, 3]` | Ordered, dynamic collection |
| `Map[K,V]` | `{a: 1, b: 2}` | Key-value store |
| `Option[T]` | `some(42) / none` | Nullable value — safe by design |
| `Result[T]` | `ok(v) / fail(e)` | Error handling — no exceptions |
| `Fn` | `fn(x) -> x * 2` | First-class function |

### Type Inference

```vel
let x = 42           // inferred: Int
let s = "hello"      // inferred: Text
let l = [1, 2, 3]   // inferred: List[Int]

// Explicit when needed
let score: Float = 98
let users: List[User] = []
```

---

## 4. Control Flow

### 4.1 If / Else

```vel
if age >= 18:
    say "Welcome"
else if age >= 13:
    say "Teen access"
else:
    say "Come back later"
```

### 4.2 Match

Match on literals:

```vel
match status:
    "active"  -> processUser(user)
    "banned"  -> rejectUser(user)
    _         -> say "Unknown status"
```

Match on expressions (VEL-style FizzBuzz):

```vel
fn fb(n: Int) -> Text:
    match true:
        n % 15 == 0 -> "FizzBuzz"
        n % 3  == 0 -> "Fizz"
        n % 5  == 0 -> "Buzz"
        _           -> toText(n)
```

Match on Result/Option:

```vel
match divide(10.0, 0.0):
    ok(val)  -> say "Result: " + val
    fail(e)  -> say "Error: " + e

match findUser(id):
    some(user) -> say user.name
    none       -> say "Not found"
```

### 4.3 Loops

```vel
// For loop
for item in cart:
    total += item.price

// For with range
for i in range(1, 11):
    say fb(i)

// While loop
while queue is not empty:
    process queue.pop()

// Repeat N times
repeat 3 times:
    ping server
```

---

## 5. Classes and Objects

Classes in VEL have no constructor boilerplate. Fields are declared directly; methods access them implicitly (no `self.` prefix needed).

```vel
class User:
    name:  Text
    email: Text
    age:   Int

    fn greet()   -> "Hi, I am " + name
    fn isAdult() -> age >= 18
```

### Instantiation

```vel
let user = User(name: "Alice", email: "a@vel.dev", age: 28)
say user.greet()      // Hi, I am Alice
say user.isAdult()    // true
```

### Inheritance

```vel
class AdminUser extends User:
    permissions: List[Text]

    fn canDelete() -> "delete" in permissions
    fn describe()  -> name + " (admin)"

let admin = AdminUser(name: "Bob", email: "b@vel.dev", age: 35, permissions: ["delete", "ban"])
say admin.greet()      // Hi, I am Bob   (inherited)
say admin.canDelete()  // true
```

---

## 6. Async and Concurrency

Every I/O operation is async by default. Use `wait` to resolve synchronously, `run` to fire-and-forget.

```vel
// Await a single async call
fn fetchUser(id: Int) -> Result[User]:
    let response = wait http.get("/users/" + id)
    -> response.as(User)

// Parallel execution — both run concurrently
let [users, products] = wait parallel:
    fetchUsers()
    fetchProducts()

// Fire-and-forget
run sendWelcomeEmail(user)

// Timeout
let data = wait fetchData() timeout 5 seconds
    or fail("Request timed out")
```

---

## 7. Error Handling

VEL uses `Result[T]` for recoverable errors. There are **no exceptions**.

### Creating Results

```vel
fn divide(a: Float, b: Float) -> Result[Float]:
    if b == 0: -> fail("Cannot divide by zero")
    -> ok(a / b)
```

### Handling Results

```vel
match divide(10.0, 2.0):
    ok(val)  -> say "Result: " + val
    fail(e)  -> say "Error: " + e
```

### Error Propagation with `?`

The `?` operator unwraps `ok(v)` to `v`, or propagates `fail(e)` up the call stack.

```vel
fn loadDashboard(userId: Int) -> Result[Dashboard]:
    let user = wait fetchUser(userId)?    // propagates if fail
    let data = wait fetchData(user.id)?   // propagates if fail
    -> ok(buildDashboard(user, data))
```

### Option Type

```vel
fn findUser(id: Int) -> Option[User]:
    if id == 1: -> some(aliceUser)
    -> none

match findUser(42):
    some(user) -> say user.name
    none       -> say "Not found"

// is checks
say ok(42) is ok      // true
say none is none      // true
say some(1) is some   // true
```

---

## 8. AI-Native Features

VEL is the first language with built-in AI inference primitives. These connect to local or cloud models through a unified adapter configured in `vel.toml`.

### Natural Language Data Queries

```vel
let inactive = wait ask users: "who has not logged in for 30 days?"
let vips     = wait ask orders: "who spent more than $1000 this month?"
let atRisk   = wait ask users: "who downgraded their plan recently?"
```

### AI Inference

```vel
let summary  = wait infer from report: "summarize key risks in 3 bullet points"
let insights = wait infer from sales: "top 3 growth opportunities for Q4"
```

### Intent-Based Routing

```vel
route userMessage:
    if feels like "billing question"  -> billingHandler(userMessage)
    if feels like "technical issue"   -> techHandler(userMessage)
    if feels like "feature request"   -> productHandler(userMessage)
    else                              -> generalHandler(userMessage)
```

### AI Agent

```vel
use vel.ai

let agent = ai.agent(
    model:   "claude-sonnet-4",
    persona: "You are a helpful support agent.",
    tools:   [db.queryTool(), email.sendTool()]
)

let reply = wait agent.handle(userMessage)
say reply
```

### Configuration

```toml
# vel.toml
[ai]
provider = "anthropic"       # or: ollama, openai, gemini
model    = "claude-sonnet-4"
```

---

## 9. Standard Library

| Module | Description |
|--------|-------------|
| `vel.text` | String manipulation, regex, templates |
| `vel.math` | Arithmetic, statistics, linear algebra |
| `vel.list` | Functional operations: map, filter, reduce, sort |
| `vel.map` | Key-value operations, merging, diffing |
| `vel.time` | Date, time, duration, timezone handling |
| `vel.fs` | File system: read, write, stream, watch |
| `vel.http` | HTTP client and server, REST, WebSocket |
| `vel.db` | Universal database adapter (SQL, NoSQL, vector) |
| `vel.ai` | AI inference, embeddings, agents, prompt management |
| `vel.crypto` | Hashing, encryption, JWT, OAuth |
| `vel.env` | Config, secrets, environment variables |
| `vel.test` | Unit testing, mocking, coverage |
| `vel.cli` | CLI framework: commands, flags, prompts |

### Built-in Functions

```vel
// Type conversion
toText(42)         // "42"
toInt("42")        // 42
toFloat("3.14")    // 3.14

// Math
abs(-5)            // 5
min(3, 7)          // 3
max(3, 7)          // 7
sqrt(16.0)         // 4.0
pow(2, 10)         // 1024.0

// Collections
range(1, 6)        // [1, 2, 3, 4, 5]
len("hello")       // 5
count([1,2,3])     // 3

// Output
say "hello"        // print to stdout
log "debug info"   // print to stderr
```

### String Methods

```vel
"hello".upper()                       // "HELLO"
"WORLD".lower()                       // "world"
"  hi  ".trim()                       // "hi"
"hello".len()                         // 5
"hello world".contains("world")       // true
"hello world".replace("world", "vel") // "hello vel"
"a,b,c".split(",")                    // ["a", "b", "c"]
["a","b","c"].join(", ")              // "a, b, c"
"hello".startsWith("hel")             // true
"hello".endsWith("llo")               // true
```

### List Methods

```vel
let nums = [3, 1, 4, 1, 5, 9, 2, 6]

nums.count()                          // 8
nums.sum()                            // 31
nums.sort()                           // [1, 1, 2, 3, 4, 5, 6, 9]
nums.reverse()                        // [6, 2, 9, 5, 1, 4, 1, 3]
nums.first()                          // some(3)
nums.last()                           // some(6)
nums.contains(4)                      // true
nums.filter(fn(n) -> n > 4)          // [5, 9, 6]
nums.map(fn(n) -> n * 2)             // [6, 2, 8, 2, 10, 18, 4, 12]
nums.reduce(fn(acc, n) -> acc + n, 0) // 31
```

---

## 10. Project Structure

```
my-project/
├── vel.toml             # Project config and dependencies
├── main.vel             # Entry point
├── src/
│   ├── models/          # Data classes
│   ├── routes/          # HTTP handlers
│   ├── services/        # Business logic
│   └── utils/           # Helpers
├── tests/
│   ├── unit/
│   └── integration/
└── build/               # Compiled output
```

### vel.toml

```toml
[project]
name    = "my-app"
version = "1.0.0"
author  = "Alice Dev"

[dependencies]
vel.http = "^3.0"
vel.db   = "^2.1"
vel.ai   = "^1.5"

[ai]
provider = "anthropic"
model    = "claude-sonnet-4"
```

---

## 11. CLI Reference

```bash
vel run   <file.vel>   # Run a program
vel check <file.vel>   # Parse-check without running
vel lex   <file.vel>   # Dump token stream (debug)
vel repl               # Interactive REPL
vel version            # Show version
```

### REPL Commands

```
vel> let x = 42
vel> say x * 2
84
vel> .exit             # quit
vel> .help             # show commands
vel> .clear            # clear screen
```

---

## Roadmap

### Phase 1 — Foundation (v0.1) — *current*
- [x] Lexer, parser, type inference
- [x] Tree-walking interpreter
- [x] Classes and inheritance
- [x] Result/Option types with pattern matching
- [x] AI-native keywords (`ask`, `infer`, `route`, `feels like`)
- [x] 81-test suite

### Phase 2 — Intelligence (v1.0)
- [ ] Real AI provider adapters (Anthropic, OpenAI, Ollama, Gemini)
- [ ] Natural language error messages
- [ ] Language Server Protocol (LSP)
- [ ] VS Code extension

### Phase 3 — Compilation (v1.5)
- [ ] LLVM-based native compilation
- [ ] WebAssembly target
- [ ] Package registry at `pkg.vel-lang.dev`

---

*© 2026 VEL Language Foundation — vel-lang.dev — MIT License*
