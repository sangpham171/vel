# Contributing to VEL

VEL is an open-source project. Contributions of all kinds are welcome.

---

## Getting Started

### Build from source

```bash
git clone https://github.com/vel-lang/vel
cd vel
make
bash tests/run_tests.sh
```

All 81 tests should pass before you begin.

### Project layout

```
vel/
├── src/
│   ├── vel.h           # Master header — all types and prototypes
│   ├── main.c          # CLI entry point
│   ├── lexer.c         # Tokenizer
│   ├── parser.c        # Recursive-descent parser → AST
│   ├── interpreter.c   # Tree-walking interpreter
│   ├── env.c           # Lexical environment (scope chain)
│   ├── value.c         # Runtime value system
│   ├── stdlib.c        # Built-in functions and methods
│   ├── error.c         # Error reporting
│   └── util.c          # Dynamic lists (NodeList, StrList)
├── examples/           # Example .vel programs
├── tests/
│   └── run_tests.sh    # Integration test suite
└── docs/               # Documentation
```

---

## How the Compiler Works

### 1. Lexer (`lexer.c`)

Source text → token stream.

```
"let x = 42" → [T_LET, T_IDENT("x"), T_EQ, T_INT(42), T_NL]
```

Key concepts:
- Indentation-based blocks: the lexer emits `T_INDENT` / `T_DEDENT` tokens
- All string values are heap-allocated (`strdup`) to avoid lifetime issues
- `push_tok` writes only one union field (ival or fval) to avoid clobbering

### 2. Parser (`parser.c`)

Token stream → AST (Abstract Syntax Tree).

- Recursive descent — each grammar rule is a function
- Pratt-style operator precedence: `or → and → not → cmp → add → mul → unary → postfix → primary`
- Pattern matching in `parse_pattern`: literals, binding names, `ok(x)`, `fail(x)`, `some(x)`, expression patterns

### 3. Interpreter (`interpreter.c`)

AST → execution.

- Tree-walking interpreter: `eval` for expressions, `exec` for statements
- `exec_list` runs a block, stopping on `SIG_RET` / `SIG_BRK`
- `call_fn` creates a new lexical scope (child env), runs the body, captures return value
- Instance methods inject field values into the method scope (implicit self)
- `match_pat` evaluates expression patterns against the subject

### 4. Environment (`env.c`)

Lexical scope chain.

```
global env → function env → block env
```

`env_get` walks up the chain. `env_def` adds to the current frame. `env_set` finds the owning frame and updates it (respecting mutability).

---

## Adding a Built-in Function

1. Add the name to `NATIVES[]` in `stdlib.c`
2. Add a case in `stdlib_call`:

```c
if (!strcmp(name, "myFunc")) {
    if (argc < 1) { vel_err(I, 0, "myFunc() needs 1 arg"); return v_nil(); }
    // ... implementation
    return v_text("result");
}
```

3. Add a test in `tests/run_tests.sh`:

```bash
chk "myFunc basic" 'say myFunc("input")' "expected output"
```

---

## Adding a Built-in Method

Add a case in `stdlib_method` in `stdlib.c`:

```c
if (obj->kind == V_TEXT) {
    if (!strcmp(m, "myMethod")) {
        // obj->sval is the string
        return v_text("result");
    }
}
```

---

## Adding a Keyword

1. Add a token in `vel.h`: `T_MYKEYWORD`
2. Add the string mapping in `kw_kind()` in `lexer.c`:
   ```c
   if (!strcmp(w, "mykeyword")) return T_MYKEYWORD;
   ```
3. Add a node type in `vel.h`: `N_MYKEYWORD`
4. Parse it in `parser.c` (`parse_stmt` or `parse_prim`)
5. Interpret it in `interpreter.c` (`eval` or `exec`)

---

## Test Guidelines

Every change must:
- Not break any existing test (`bash tests/run_tests.sh`)
- Add a new test for new behaviour

Test format:

```bash
chk "description" 'vel source code' "expected output"
```

Multi-line VEL source:

```bash
chk "multi-line test" 'fn add(a, b):
    -> a + b
say add(3, 4)' "7"
```

---

## Code Style

- C99 standard — no compiler extensions
- No external dependencies — only `libc` and `libm`
- Use `snprintf` instead of `strncpy` to avoid truncation warnings
- Every heap allocation must succeed or call `exit(1)` — no silent nulls
- Error messages: `vel_err(I, node->line, "clear message: %s", detail)`

---

## Submitting Changes

1. Fork the repository
2. Create a branch: `git checkout -b feat/my-feature`
3. Make your changes
4. Run tests: `bash tests/run_tests.sh`
5. Commit with a clear message: `feat: add string.repeat() method`
6. Open a pull request

### Commit message format

```
feat: add new feature
fix: correct a bug
docs: update documentation
test: add missing tests
refactor: restructure without changing behaviour
```

---

## Reporting Issues

Open an issue at `github.com/vel-lang/vel/issues` with:
- VEL version: `vel version`
- Minimal reproducing example
- Expected vs actual output

---

*© 2026 VEL Language Foundation — vel-lang.dev*
