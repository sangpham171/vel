# VEL Cookbook

Practical patterns for real-world VEL programs.

---

## 1. REST API Server

```vel
use vel.http
use vel.db

let app = http.server()
let db  = database.connect(env.get("DATABASE_URL"))

app.get("/users", fn(req):
    let users = wait db.all(User)
    -> users.toJson())

app.get("/users/:id", fn(req):
    let user = wait db.find(User, req.params.id)?
    -> user.toJson())

app.post("/users", fn(req):
    let user  = req.body.as(User)?
    let saved = wait db.save(user)?
    -> saved.toJson())

app.delete("/users/:id", fn(req):
    wait db.delete(User, req.params.id)?
    -> {ok: true})

app.listen(3000)
say "Server running on port 3000"
```

---

## 2. AI-Powered Support Bot

```vel
use vel.ai
use vel.http

let bot = ai.agent(
    model:   "claude-sonnet-4",
    persona: "You are a helpful and concise support agent.",
    tools:   [db.queryTool(), email.sendTool()]
)

fn handleSupport(message: Text) -> Text:
    route message:
        if feels like "billing"         -> billingAgent(message)
        if feels like "technical issue" -> techAgent(message)
        if feels like "refund"          -> refundAgent(message)
        else                            -> wait bot.reply(message)

let app = http.server()

app.post("/support", fn(req):
    let reply = wait handleSupport(req.body.text)
    -> {reply: reply})

app.listen(8080)
```

---

## 3. File Processing Pipeline

```vel
use vel.fs

fn processLogs(inputPath: Text, outputPath: Text) -> Result[Int]:
    let lines  = wait fs.readLines(inputPath)?
    let errors = lines
        .filter(fn(l) -> l.contains("ERROR"))
        .map(fn(l) -> l.after("ERROR: ").trim())

    wait fs.write(outputPath, errors.join("\n"))?
    -> ok(errors.count())

match processLogs("app.log", "errors.txt"):
    ok(count) -> say "Extracted " + count + " errors"
    fail(e)   -> say "Failed: " + e
```

---

## 4. Functional List Operations

```vel
let scores = [92, 45, 87, 61, 95, 73, 88, 52]

// Filter passing scores, scale, and rank
let ranked = scores
    .filter(fn(s) -> s >= 60)
    .map(fn(s) -> s * 1.05)
    .sort()
    .reverse()

say ranked

// Aggregate stats
say "Count:   " + scores.count()
say "Sum:     " + scores.sum()
say "Highest: " + scores.sort().last().unwrap()
say "Passing: " + scores.filter(fn(s) -> s >= 60).count()

// Group by grade
fn grade(s: Int) -> Text:
    match true:
        s >= 90 -> "A"
        s >= 80 -> "B"
        s >= 70 -> "C"
        s >= 60 -> "D"
        _       -> "F"

for s in scores:
    say toText(s) + " -> " + grade(s)
```

---

## 5. Database Access

```vel
use vel.db

class Product:
    id:       Int
    name:     Text
    price:    Float
    category: Text
    stock:    Int

fn findCheapProducts(maxPrice: Float) -> Result[List[Product]]:
    -> ok(wait db.query(
        "SELECT * FROM products WHERE price < ? AND stock > 0",
        [maxPrice]
    ).as(List[Product]))

fn updateStock(productId: Int, delta: Int) -> Result[Product]:
    let product = wait db.find(Product, productId)?
    product.stock += delta
    -> ok(wait db.save(product)?)

// AI-powered query
let trending = wait ask products: "which items have increased sales this week?"
```

---

## 6. Concurrent Data Fetching

```vel
use vel.http

fn fetchDashboard(userId: Int) -> Result[Map]:
    // All three run concurrently
    let [user, orders, recommendations] = wait parallel:
        fetchUser(userId)
        fetchOrders(userId)
        fetchRecommendations(userId)

    -> ok({
        user:            user,
        recentOrders:    orders,
        recommendations: recommendations,
        fetchedAt:       now()
    })

match fetchDashboard(42):
    ok(data)  -> say data.toJson()
    fail(e)   -> say "Dashboard error: " + e
```

---

## 7. CLI Tool

```vel
use vel.cli

let cmd = cli.program(
    name:        "vel-deploy",
    version:     "1.0.0",
    description: "Deploy VEL applications"
)

cmd.command("deploy", fn(args):
    let env  = args.get("env")  or "staging"
    let tag  = args.get("tag")  or "latest"
    say "Deploying " + tag + " to " + env + "..."
    wait deployApp(env, tag))

cmd.command("rollback", fn(args):
    let env = args.get("env") or "staging"
    say "Rolling back " + env + "..."
    wait rollbackApp(env))

cmd.run()
```

---

## 8. Testing

```vel
use vel.test

class Calculator:
    fn add(a: Int, b: Int)      -> a + b
    fn divide(a: Float, b: Float) -> Result[Float]:
        if b == 0: -> fail("Division by zero")
        -> ok(a / b)

let calc = Calculator()

test "addition is correct":
    expect calc.add(2, 3) == 5
    expect calc.add(-1, 1) == 0
    expect calc.add(0, 0) == 0

test "division works":
    match calc.divide(10.0, 2.0):
        ok(v)   -> expect v == 5.0
        fail(e) -> fail("Should not fail")

test "division by zero returns fail":
    expect calc.divide(10.0, 0.0) is fail
```

---

## 9. FizzBuzz

The classic, VEL-style:

```vel
fn fb(n: Int) -> Text:
    match true:
        n % 15 == 0 -> "FizzBuzz"
        n % 3  == 0 -> "Fizz"
        n % 5  == 0 -> "Buzz"
        _           -> toText(n)

for i in range(1, 31):
    say fb(i)
```

---

## 10. Configuration and Environment

```vel
use vel.env

// Required values — fail fast if missing
let dbUrl     = env.require("DATABASE_URL")
let apiKey    = env.require("API_KEY")

// Optional with defaults
let port      = env.get("PORT")    or "3000"
let logLevel  = env.get("LOG_LEVEL") or "info"
let maxConns  = toInt(env.get("MAX_CONNECTIONS") or "10")

say "Starting on port " + port
say "Log level: " + logLevel
say "Max connections: " + maxConns
```

---

## 11. Error Propagation Chain

```vel
fn loadUser(id: Int) -> Result[User]:
    let row = wait db.findOne("users", id)?
    -> ok(row.as(User))

fn loadUserPrefs(userId: Int) -> Result[Prefs]:
    let row = wait db.findOne("preferences", userId)?
    -> ok(row.as(Prefs))

fn buildUserContext(id: Int) -> Result[Context]:
    let user  = wait loadUser(id)?
    let prefs = wait loadUserPrefs(user.id)?
    let feed  = wait generateFeed(user, prefs)?
    -> ok(Context(user: user, prefs: prefs, feed: feed))

// All errors bubble up cleanly
match buildUserContext(42):
    ok(ctx)  -> renderDashboard(ctx)
    fail(e)  -> say "Could not load context: " + e
```

---

## 12. AI Data Analysis

```vel
use vel.ai
use vel.db

fn weeklyReport() -> Result[Text]:
    // Natural language queries over structured data
    let churnRisk  = wait ask users: "who is likely to cancel this month?"
    let topSpenders = wait ask orders: "top 10 customers by revenue this quarter"
    let issues     = wait ask tickets: "unresolved issues open more than 7 days"

    // Synthesize into a report
    let summary = wait infer from {
        atRisk:      churnRisk,
        topCustomers: topSpenders,
        openIssues:  issues
    }: "write an executive summary with 3 action items"

    -> ok(summary)

match weeklyReport():
    ok(report) -> wait email.send(to: "ceo@company.com", subject: "Weekly Report", body: report)
    fail(e)    -> log "Report failed: " + e
```

---

*© 2026 VEL Language Foundation — vel-lang.dev*
