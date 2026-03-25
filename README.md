# VEL

**The volitional programming language.**

> Express intent. The language figures out the rest.

[![License: MIT](https://img.shields.io/badge/License-MIT-violet.svg)](LICENSE)
[![Version](https://img.shields.io/badge/version-0.1.0-blueviolet)]()
[![Build](https://img.shields.io/badge/build-C99%20pure-orange)]()
[![Dependencies](https://img.shields.io/badge/dependencies-zero-brightgreen)]()

---

## What is VEL?

VEL (*from Latin velle* — to will, to intend) is a programming language built around a single idea: **you should express what you want, not how to achieve it.**

```vel
// A complete REST API endpoint
use vel.http, vel.db

class User:
    name:  Text
    email: Text
    age:   Int

let app = http.server()

app.get("/users/:id", fn(req):
    let user = wait db.find(User, req.params.id)?
    -> user.toJson())

app.listen(3000)
```

```vel
// AI-native intent queries
let inactive = wait ask users: "who has not logged in for 30 days?"
let summary  = wait infer from report: "summarize top 3 risks"

route message:
    if feels like "billing"  -> billingHandler(message)
    if feels like "support"  -> supportHandler(message)
    else                     -> generalHandler(message)
```

---

## Design Principles

| Principle | Description |
|-----------|-------------|
| **Volitional** | Code expresses intent — the runtime resolves how |
| **Zero ceremony** | No boilerplate, no mandatory types, no semicolons |
| **AI-native** | `ask`, `infer`, `feels like` are first-class keywords |
| **Memory-safe** | Automatic ownership — no leaks, no manual management |
| **Concurrent by default** | Every I/O call is async; `parallel:` is one keyword |
| **Deploy anywhere** | Native binary, WebAssembly, or interpreted |

---

## Syntax Tour

### Variables
```vel
let name   = "Alice"       // mutable, type inferred
let age    = 28
fix PI     = 3.14159       // immutable constant
let score: Float = 98.5   // explicit annotation when needed
```

### Functions
```vel
fn greet(name: Text) -> "Hello, " + name

fn discount(price: Float, rate: Float) -> Float:
    let saving = price * rate / 100
    -> price - saving
```

### Classes
```vel
class User:
    name:  Text
    email: Text
    age:   Int

    fn greet()   -> "Hi, I'm " + name
    fn isAdult() -> age >= 18

class AdminUser extends User:
    permissions: List[Text]
    fn canDelete() -> "delete" in permissions
```

### Async & Concurrency
```vel
let user = wait fetchUser(42)

let [users, products] = wait parallel:
    fetchUsers()
    fetchProducts()

run sendWelcomeEmail(user)   // fire-and-forget
```

### Error Handling
```vel
fn divide(a: Float, b: Float) -> Result[Float]:
    if b == 0: -> fail("Cannot divide by zero")
    -> ok(a / b)

match divide(10.0, 0.0):
    ok(val)  -> say "Result: " + val
    fail(e)  -> say "Error: " + e

fn loadDashboard(id: Int) -> Result[Dashboard]:
    let user = wait fetchUser(id)?
    let data = wait fetchData(user.id)?
    -> buildDashboard(user, data)
```

### AI-Native Features
```vel
// Natural language data queries
let vips = wait ask users: "who spent more than $1000 this month"

// AI inference from structured data
let insights = wait infer from sales: "top 3 growth opportunities"

// Semantic routing
route userMessage:
    if feels like "technical issue"  -> techSupport(userMessage)
    if feels like "refund request"   -> billing(userMessage)
    else                             -> general(userMessage)
```

---


## Documentation

Full documentation lives in the [](docs/) folder.

| Document | Description |
|----------|-------------|
| [Quick Start](docs/quick-start.md) | From zero to running VEL in 5 minutes |
| [Language Specification](docs/language-spec.md) | Complete syntax and semantics reference |
| [Standard Library](docs/stdlib.md) | All built-in functions, methods, and modules |
| [Cookbook](docs/cookbook.md) | Real-world patterns and examples |
| [Contributing](docs/contributing.md) | Architecture and contribution guide |

## Build

```bash
# Zero dependencies — just a C99 compiler
make

# Or directly:
cc -std=c99 -O2 -o vel src/*.c -lm
```

### Requirements
- Any C99 compiler: `gcc`, `clang`, `tcc`, `msvc`
- `libc` + `libm` (always available)
- Nothing else

---

## Usage

```bash
vel run   file.vel      # Run a program
vel check file.vel      # Parse-check only
vel lex   file.vel      # Dump token stream
vel repl                # Interactive REPL
vel version             # Show version
```

---

## Project Structure

```
vel/
├── src/
│   ├── vel.h           Master header — all types and prototypes
│   ├── main.c          CLI entry point
│   ├── lexer.c         Tokenizer — 70+ token types
│   ├── parser.c        Recursive-descent parser → AST
│   ├── interpreter.c   Tree-walking interpreter
│   ├── env.c           Lexical environment (scope chain)
│   ├── value.c         Runtime value system
│   ├── stdlib.c        40+ built-in functions and methods
│   ├── error.c         Error reporting
│   └── util.c          Dynamic lists
├── examples/           Example .vel programs
├── tests/
│   └── run_tests.sh    Test suite (81 tests)
├── Makefile
└── README.md
```

---

## Test Results

```
Results: 81 passed, 0 failed / 81 total
```

Run: `make test` or `bash tests/run_tests.sh`

---

## Roadmap

### Phase 1 — Foundation (v0.1) ← current
- [x] Lexer, parser, type inference
- [x] Tree-walking interpreter
- [x] Classes and inheritance
- [x] Result/Option types
- [x] AI-native keywords (`ask`, `infer`, `route`, `feels like`)
- [x] 81-test suite

### Phase 2 — Intelligence (v1.0)
- [ ] Real AI provider integration (local + cloud)
- [ ] Natural language error messages
- [ ] Language server protocol (LSP)
- [ ] VS Code extension

### Phase 3 — Compilation (v1.5)
- [ ] LLVM-based native compilation
- [ ] WebAssembly target
- [ ] Package registry at pkg.vel-lang.dev

---

## License

MIT — see [LICENSE](LICENSE)

---

*© 2026 VEL Language Foundation — vel-lang.dev*
