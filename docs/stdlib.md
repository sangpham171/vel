# VEL Standard Library Reference

Complete reference for all built-in functions, methods, and modules.

---

## Built-in Functions

### I/O

| Function | Signature | Description |
|----------|-----------|-------------|
| `say` | `say value` | Print to stdout with newline |
| `log` | `log value` | Print to stderr with newline |

```vel
say "Hello"               // Hello
say 42                    // 42
say [1, 2, 3]             // [1, 2, 3]
log "debug: x = " + x    // to stderr
```

### Type Conversion

| Function | Signature | Description |
|----------|-----------|-------------|
| `toText` | `toText(v: Any) -> Text` | Convert any value to Text |
| `toInt` | `toInt(v: Any) -> Int` | Convert to Int (fails if invalid) |
| `toFloat` | `toFloat(v: Any) -> Float` | Convert to Float |
| `toBool` | `toBool(v: Any) -> Bool` | Truthy conversion |
| `type_of` | `type_of(v: Any) -> Text` | Return type name as Text |

```vel
toText(42)          // "42"
toText(3.14)        // "3.14"
toText(true)        // "true"
toText([1,2,3])     // "[1, 2, 3]"
toInt("42")         // 42
toFloat("3.14")     // 3.14
type_of(42)         // "Int"
type_of("hello")    // "Text"
type_of([1,2])      // "List"
```

### Math

| Function | Signature | Description |
|----------|-----------|-------------|
| `abs` | `abs(n) -> n` | Absolute value |
| `min` | `min(a, b) -> n` | Minimum of two values |
| `max` | `max(a, b) -> n` | Maximum of two values |
| `floor` | `floor(f: Float) -> Int` | Round down |
| `ceil` | `ceil(f: Float) -> Int` | Round up |
| `round` | `round(f: Float) -> Int` | Round to nearest |
| `sqrt` | `sqrt(f: Float) -> Float` | Square root |
| `pow` | `pow(base, exp) -> Float` | Exponentiation |

```vel
abs(-7)         // 7
min(3, 9)       // 3
max(3, 9)       // 9
floor(3.7)      // 3
ceil(3.2)       // 4
round(3.5)      // 4
sqrt(16.0)      // 4.0
pow(2, 8)       // 256.0
```

### Collections

| Function | Signature | Description |
|----------|-----------|-------------|
| `len` | `len(v) -> Int` | Length of Text, List, or Map |
| `count` | `count(v) -> Int` | Alias for `len` |
| `range` | `range(start, end) -> List[Int]` | Integer range (exclusive end) |
| `first` | `first(list) -> Option[T]` | First element |
| `last` | `last(list) -> Option[T]` | Last element |
| `reverse` | `reverse(v) -> v` | Reverse List or Text |
| `keys` | `keys(map) -> List` | Map keys |
| `values` | `values(map) -> List` | Map values |

```vel
len("hello")        // 5
len([1, 2, 3])      // 3
range(1, 5)         // [1, 2, 3, 4]
first([10, 20, 30]) // some(10)
last([10, 20, 30])  // some(30)
reverse([1,2,3])    // [3, 2, 1]
reverse("abc")      // "cba"
```

---

## Text Methods

```vel
let s = "Hello, World!"
```

| Method | Result | Description |
|--------|--------|-------------|
| `s.len()` | `13` | Character count |
| `s.upper()` | `"HELLO, WORLD!"` | Uppercase |
| `s.lower()` | `"hello, world!"` | Lowercase |
| `s.trim()` | `"Hello, World!"` | Strip whitespace |
| `s.contains("World")` | `true` | Substring check |
| `s.startsWith("Hello")` | `true` | Prefix check |
| `s.endsWith("!")` | `true` | Suffix check |
| `s.replace("World", "VEL")` | `"Hello, VEL!"` | Replace all occurrences |
| `s.split(", ")` | `["Hello", "World!"]` | Split into list |
| `s.after("Hello, ")` | `"World!"` | Substring after separator |
| `s.toJson()` | `'"Hello, World!"'` | JSON-encoded string |
| `s.reverse()` | `"!dlroW ,olleH"` | Reverse characters |

---

## List Methods

```vel
let nums = [3, 1, 4, 1, 5, 9]
```

| Method | Result | Description |
|--------|--------|-------------|
| `nums.count()` | `6` | Number of elements |
| `nums.len()` | `6` | Alias for count |
| `nums.sum()` | `23` | Sum (requires numeric list) |
| `nums.sort()` | `[1, 1, 3, 4, 5, 9]` | Sort ascending |
| `nums.reverse()` | `[9, 5, 1, 4, 1, 3]` | Reverse order |
| `nums.first()` | `some(3)` | First element as Option |
| `nums.last()` | `some(9)` | Last element as Option |
| `nums.contains(4)` | `true` | Membership check |
| `nums.join(", ")` | `"3, 1, 4, 1, 5, 9"` | Join into Text |
| `nums.filter(fn(n) -> n > 3)` | `[4, 5, 9]` | Keep matching |
| `nums.map(fn(n) -> n * 2)` | `[6, 2, 8, 2, 10, 18]` | Transform each |
| `nums.reduce(fn(a,b) -> a+b, 0)` | `23` | Fold to single value |
| `nums.toJson()` | `"[3, 1, 4, 1, 5, 9]"` | JSON array |

---

## Map Methods

```vel
let m = {name: "Alice", age: 28}
```

| Method | Result | Description |
|--------|--------|-------------|
| `m.count()` | `2` | Number of entries |
| `m.keys()` | `["name", "age"]` | List of keys |
| `m.values()` | `["Alice", 28]` | List of values |
| `m.has("name")` | `true` | Key existence check |

---

## Result Methods

```vel
let r = ok(42)
let e = fail("not found")
```

| Method | Description |
|--------|-------------|
| `r.unwrap()` | Extract value — panics on fail |
| `r is ok` | Check if successful |
| `r is fail` | Check if failed |

```vel
match r:
    ok(v)   -> say v         // pattern match (preferred)
    fail(e) -> say e

// Or unwrap when you're certain
let v = ok(42).unwrap()      // 42
```

---

## Option Methods

```vel
let s = some(42)
let n = none
```

| Method | Description |
|--------|-------------|
| `s.unwrap()` | Extract value — panics on none |
| `s is some` | Check if present |
| `n is none` | Check if absent |

```vel
match findUser(id):
    some(user) -> say user.name   // pattern match (preferred)
    none       -> say "not found"
```

---

## Modules

### vel.http

```vel
use vel.http

let app = http.server()

app.get("/path",           fn(req) -> response)
app.post("/path",          fn(req) -> response)
app.put("/path/:id",       fn(req) -> response)
app.delete("/path/:id",    fn(req) -> response)

// Request properties
req.params.id          // URL parameter
req.query.get("sort")  // Query string
req.body               // Request body
req.headers.get("Authorization")

// Response helpers
-> {data: value}       // JSON response
-> req.body.toJson()   // Echo body
-> http.redirect("/")  // Redirect

app.listen(3000)
```

### vel.db

```vel
use vel.db

let db = database.connect(env.get("DATABASE_URL"))

// CRUD
wait db.all(User)
wait db.find(User, id)
wait db.findOne("users", id)
wait db.save(user)
wait db.delete(User, id)

// Raw query
wait db.query("SELECT * FROM users WHERE age > ?", [18]).as(List[User])

// AI query
wait ask users: "who signed up in the last 7 days?"
```

### vel.fs

```vel
use vel.fs

let content = wait fs.read("file.txt")
let lines   = wait fs.readLines("file.txt")
wait fs.write("output.txt", content)
wait fs.append("log.txt", newLine)

let exists = wait fs.exists("config.vel")
let files  = wait fs.list("./src")
```

### vel.ai

```vel
use vel.ai

// Simple completion
let reply = wait ai.complete("Summarize this text: " + content)

// Agent with tools
let agent = ai.agent(
    model:   "claude-sonnet-4",
    persona: "You are a helpful assistant.",
    tools:   [db.queryTool()]
)
let response = wait agent.handle(userMessage)

// Embeddings
let embedding = wait ai.embed("VEL programming language")
```

### vel.test

```vel
use vel.test

test "description":
    expect value == expected
    expect result is ok
    expect list.contains(item)
    expect fn() -> value  // lazy evaluation

// Mocking
let mock = mock.http("/api/users", response: sampleData)
```

### vel.time

```vel
use vel.time

let now     = time.now()
let today   = time.today()
let ts      = time.parse("2026-01-01", "YYYY-MM-DD")
let later   = now.add(7, "days")
let diff    = later.diff(now, "hours")
let iso     = now.format("ISO8601")
```

### vel.crypto

```vel
use vel.crypto

let hash    = crypto.hash("sha256", data)
let token   = crypto.jwt.sign(payload, secret)
let claims  = crypto.jwt.verify(token, secret)
let bcrypt  = wait crypto.bcrypt.hash(password)
let matches = wait crypto.bcrypt.verify(password, bcrypt)
```

### vel.env

```vel
use vel.env

let value   = env.get("KEY")              // Option[Text]
let value2  = env.get("KEY") or "default"
let required = env.require("SECRET_KEY") // fails if missing
```

---

## Pattern Matching Reference

```vel
// Match on type variants
match result:
    ok(v)   -> use v
    fail(e) -> handle e

match option:
    some(v) -> use v
    none    -> handle absence

// Match on values
match x:
    0       -> "zero"
    1       -> "one"
    _       -> "other"

// Match on expressions
match true:
    x > 100 -> "high"
    x > 50  -> "medium"
    _       -> "low"

// Match on strings
match status:
    "active"   -> process()
    "inactive" -> archive()
    _          -> log "unknown"

// Binding in patterns
match findUser(id):
    some(user) -> say user.name    // user is bound
    none       -> say "not found"

match fetchData():
    ok(data) -> process(data)     // data is bound
    fail(e)  -> say "error: " + e // e is bound
```

---

## Operator Reference

| Operator | Type | Description |
|----------|------|-------------|
| `+` | arithmetic | Addition (also string concat) |
| `-` | arithmetic | Subtraction |
| `*` | arithmetic | Multiplication (also string repeat) |
| `/` | arithmetic | Division |
| `%` | arithmetic | Modulo |
| `==` | comparison | Equality |
| `!=` | comparison | Inequality |
| `<` | comparison | Less than |
| `<=` | comparison | Less than or equal |
| `>` | comparison | Greater than |
| `>=` | comparison | Greater than or equal |
| `and` | logical | Logical AND |
| `or` | logical | Logical OR |
| `not` | logical | Logical NOT |
| `in` | membership | Element in List or Text |
| `is` | type check | `x is ok`, `x is some`, etc. |
| `->` | return | Return a value from a function |
| `?` | propagation | Unwrap ok or propagate fail |
| `+=` | assignment | Add and assign |
| `-=` | assignment | Subtract and assign |

---

*© 2026 VEL Language Foundation — vel-lang.dev*
