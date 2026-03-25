#!/bin/bash
# VEL Test Suite
# Works on macOS and Linux — no external dependencies

# ── Locate project root ──────────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
cd "$ROOT"

# ── Always recompile from source ─────────────────────────────────────────────
echo "Building VEL..."
make clean -s
make -s
echo ""

VEL="$ROOT/vel"
if [ ! -x "$VEL" ]; then
    echo "ERROR: Build failed — $VEL not found"
    exit 1
fi

echo "Using: $($VEL version 2>/dev/null)"
echo ""

# ── Portable timeout (works on macOS and Linux) ───────────────────────────────
# macOS does not ship GNU timeout — use a pure-bash alternative
vel_run() {
    local file="$1"
    # Run in background, kill after 5 seconds if hung
    "$VEL" run "$file" 2>/dev/null &
    local pid=$!
    local i=0
    while [ $i -lt 50 ]; do   # 50 × 0.1s = 5s
        sleep 0.1
        if ! kill -0 $pid 2>/dev/null; then
            wait $pid
            return $?
        fi
        i=$((i+1))
    done
    kill $pid 2>/dev/null
    wait $pid 2>/dev/null
    return 124   # timeout exit code
}

# ── Test helpers ─────────────────────────────────────────────────────────────
PASS=0; FAIL=0; TOTAL=0
TMP=$(mktemp /tmp/vel_test_XXXXXX.vel)
trap 'rm -f "$TMP"' EXIT

pass() { printf "\033[32mPASS\033[0m %s\n" "$1"; PASS=$((PASS+1)); TOTAL=$((TOTAL+1)); }
fail() {
    printf "\033[31mFAIL\033[0m %s\n  expected: |%s|\n  got:      |%s|\n" "$1" "$2" "$3"
    FAIL=$((FAIL+1)); TOTAL=$((TOTAL+1))
}

chk() {
    local desc="$1" expected="$3"
    printf '%s' "$2" > "$TMP"
    local got
    got=$(vel_run "$TMP")
    local code=$?
    if [ $code -eq 124 ]; then
        fail "$desc" "$expected" "(timeout)"
    elif [ "$got" = "$expected" ]; then
        pass "$desc"
    else
        fail "$desc" "$expected" "$got"
    fi
}

echo "-- VEL Test Suite --------------------------------------------"
echo ""

# ── Literals ─────────────────────────────────────────────────────────────────
chk "integer"         'say 42'                    "42"
chk "float"           'say 3.14'                  "3.14"
chk "string"          'say "hello"'               "hello"
chk "bool true"       'say true'                  "true"
chk "bool false"      'say false'                 "false"

# ── Arithmetic ───────────────────────────────────────────────────────────────
chk "add"             'say 2 + 3'                 "5"
chk "subtract"        'say 10 - 4'                "6"
chk "multiply"        'say 6 * 7'                 "42"
chk "divide"          'say 10 / 2'                "5"
chk "modulo"          'say 10 % 3'                "1"
chk "precedence"      'say 2 + 3 * 4'             "14"
chk "parentheses"     'say (2 + 3) * 4'           "20"
chk "negate"          'say 0 - 5'                 "-5"

# ── Strings ──────────────────────────────────────────────────────────────────
chk "str concat"      'say "hello" + " world"'    "hello world"
chk "str + int"       'say "x=" + 42'             "x=42"
chk "upper"           'say "hello".upper()'        "HELLO"
chk "lower"           'say "WORLD".lower()'        "world"
chk "trim"            'say "  hi  ".trim()'        "hi"
chk "str len"         'say "hello".len()'          "5"
chk "contains true"   'say "vel".contains("el")'   "true"
chk "contains false"  'say "vel".contains("xyz")'  "false"
chk "replace"         'say "hello world".replace("world","vel")' "hello vel"
chk "starts with"     'say "hello".startsWith("hel")' "true"
chk "ends with"       'say "hello".endsWith("llo")'   "true"
chk "split"           'say "a,b".split(",")' "[a, b]"

# ── Variables ─────────────────────────────────────────────────────────────────
chk "let"             'let x = 42
say x' "42"

chk "fix"             'fix PI = 3.14
say PI' "3.14"

chk "reassign"        'let x = 1
x = 2
say x' "2"

chk "plus-eq"         'let x = 10
x += 5
say x' "15"

chk "minus-eq"        'let x = 10
x -= 3
say x' "7"

# ── Functions ─────────────────────────────────────────────────────────────────
chk "fn one-liner"    'fn dbl(n) -> n * 2
say dbl(21)' "42"

chk "fn body"         'fn add(a, b):
    -> a + b
say add(10, 32)' "42"

chk "fn recursive"    'fn fac(n):
    if n <= 1: -> 1
    -> n * fac(n - 1)
say fac(5)' "120"

chk "fn closure"      'fn makeAdder(x):
    -> fn(y) -> x + y
let add5 = makeAdder(5)
say add5(10)' "15"

chk "lambda"          'let triple = fn(x) -> x * 3
say triple(7)' "21"

# ── Control flow ──────────────────────────────────────────────────────────────
chk "if true"         'if true:
    say "yes"' "yes"

chk "if else"         'if false:
    say "yes"
else:
    say "no"' "no"

chk "elif"            'let x = 5
if x > 10:
    say "big"
else if x > 3:
    say "medium"
else:
    say "small"' "medium"

chk "for list"        'let t = 0
for i in [1, 2, 3, 4, 5]:
    t += i
say t' "15"

chk "for range"       'let t = 0
for i in range(1, 6):
    t += i
say t' "15"

chk "while"           'let n = 1
while n < 10:
    n = n * 2
say n' "16"

chk "repeat"          'let c = 0
repeat 5 times:
    c += 1
say c' "5"

# ── Match ─────────────────────────────────────────────────────────────────────
chk "match literal"   'fn lbl(n):
    match n:
        1 -> "one"
        2 -> "two"
        _ -> "other"
say lbl(2)
say lbl(9)' "two
other"

chk "match expr"      'fn fb(n):
    match true:
        n % 3 == 0 -> "Fizz"
        n % 5 == 0 -> "Buzz"
        _ -> toText(n)
say fb(3)
say fb(5)
say fb(7)' "Fizz
Buzz
7"

chk "match result"    'fn chk2(n):
    if n > 0: -> ok("pos")
    -> fail("neg")
match chk2(5):
    ok(v)   -> say v
    fail(e) -> say e
match chk2(-1):
    ok(v)   -> say v
    fail(e) -> say e' "pos
neg"

# ── Collections ───────────────────────────────────────────────────────────────
chk "list literal"    'say [1, 2, 3]'              "[1, 2, 3]"
chk "list count"      'say [1, 2, 3].count()'      "3"
chk "list index"      'say [10, 20, 30][1]'        "20"
chk "list sum"        'say [1, 2, 3, 4, 5].sum()'  "15"
chk "list join"       'say ["a","b","c"].join(", ")' "a, b, c"
chk "list sort"       'say [3, 1, 2].sort()'       "[1, 2, 3]"
chk "list reverse"    'say [1, 2, 3].reverse()'    "[3, 2, 1]"
chk "list filter"     'say [1,2,3,4,5].filter(fn(n) -> n > 3)' "[4, 5]"
chk "list map"        'say [1,2,3].map(fn(n) -> n * 10)'        "[10, 20, 30]"
chk "list reduce"     'say [1,2,3,4].reduce(fn(a,b) -> a+b, 0)' "10"

# ── Result / Option ───────────────────────────────────────────────────────────
chk "ok"              'say ok(42)'          "ok(42)"
chk "fail"            'say fail("oops")'    'fail("oops")'
chk "some"            'say some(99)'        "some(99)"
chk "none"            'say none'            "none"
chk "is ok"           'say ok(1) is ok'     "true"
chk "is fail"         'say fail("e") is fail' "true"
chk "is some"         'say some(1) is some' "true"
chk "is none"         'say none is none'    "true"

# ── Comparisons ───────────────────────────────────────────────────────────────
chk "equality"        'say 1 == 1'          "true"
chk "inequality"      'say 1 != 2'          "true"
chk "less than"       'say 3 < 5'           "true"
chk "lteq"            'say 3 <= 3'          "true"
chk "greater than"    'say 5 > 3'           "true"
chk "gteq"            'say 5 >= 5'          "true"
chk "and"             'say true and true'   "true"
chk "or"              'say false or true'   "true"
chk "not"             'say not false'       "true"
chk "in list true"    'say 3 in [1,2,3]'   "true"
chk "in list false"   'say 9 in [1,2,3]'   "false"
chk "in string"       'say "vel" in "vel-lang"' "true"

# ── Classes ───────────────────────────────────────────────────────────────────
chk "class basic"     'class Dog:
    name: Text
    fn bark() -> name + " says woof"
let d = Dog("Rex")
say d.bark()' "Rex says woof"

chk "class fields"    'class Point:
    x: Int
    y: Int
    fn str() -> "(" + x + "," + y + ")"
let p = Point(3, 4)
say p.str()' "(3,4)"

chk "inheritance"     'class Animal:
    name: Text
    fn greet() -> "Hi from " + name
class Dog extends Animal:
    breed: Text
    fn bark() -> name + " barks"
let d = Dog("Rex", "Lab")
say d.greet()
say d.bark()' "Hi from Rex
Rex barks"

# ── AI features ───────────────────────────────────────────────────────────────
chk "route match"     'route "billing question":
    if feels like "billing" -> say "billing"
    if feels like "support" -> say "support"
    else -> say "other"' "billing"

chk "route else"      'route "hello there":
    if feels like "billing" -> say "billing"
    else -> say "other"' "other"

# ── FizzBuzz ──────────────────────────────────────────────────────────────────
chk "fizzbuzz"        'fn fb(n):
    match true:
        n % 15 == 0 -> "FizzBuzz"
        n % 3  == 0 -> "Fizz"
        n % 5  == 0 -> "Buzz"
        _           -> toText(n)
for i in range(1, 16):
    say fb(i)' "1
2
Fizz
4
Buzz
Fizz
7
8
Fizz
Buzz
11
Fizz
13
14
FizzBuzz"

# ── Summary ───────────────────────────────────────────────────────────────────
echo ""
echo "--------------------------------------------------------------"
printf "Results: \033[32m%d passed\033[0m, \033[31m%d failed\033[0m / %d total\n" \
       $PASS $FAIL $TOTAL
echo ""
[ $FAIL -eq 0 ] && exit 0 || exit 1
