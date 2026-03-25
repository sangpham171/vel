# VEL Documentation

Welcome to the VEL language documentation.

---

## Guides

| Document | Description |
|----------|-------------|
| [Quick Start](quick-start.md) | From zero to running VEL in 5 minutes |
| [Language Specification](language-spec.md) | Complete syntax and semantics reference |
| [Standard Library](stdlib.md) | Built-in functions, methods, and modules |
| [Cookbook](cookbook.md) | Real-world patterns and examples |
| [Contributing](contributing.md) | How the compiler works, how to contribute |

---

## Quick Reference

```vel
// Variables
let x   = 42
fix PI  = 3.14159

// Functions
fn greet(name: Text) -> "Hello, " + name

// Classes
class User:
    name: Text
    fn welcome() -> "Welcome, " + name

// Error handling
fn divide(a: Float, b: Float) -> Result[Float]:
    if b == 0: -> fail("Division by zero")
    -> ok(a / b)

// AI features
route message:
    if feels like "billing" -> billingHandler(message)
    else                    -> generalHandler(message)
```

---

## Build

```bash
make
./vel run examples/hello.vel
```

---

*© 2026 VEL Language Foundation — [vel-lang.dev](https://vel-lang.dev) — MIT License*
