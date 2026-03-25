/*
 * vel.h — VEL language: lean, pointer-based design
 * Pure C99. Zero external dependencies.
 * Build: gcc -std=c99 -O2 -o vel src/main.c src/util.c src/lexer.c \
 *        src/parser.c src/interpreter.c src/env.c src/value.c \
 *        src/stdlib.c src/error.c -lm
 */
#ifndef VEL_H
#define VEL_H

/* Expose POSIX extensions (strdup, etc.) in C99 mode */
#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>

typedef long long      i64;
typedef unsigned char  u8;
#ifndef bool
#  define bool  int
#  define true  1
#  define false 0
#endif

/* ── Limits (all small / sane) ─────────────────────────────────────────── */
#define MAX_IDENT      128
#define MAX_STR       4096
#define MAX_TOKENS    8192
#define MAX_PARAMS      32
#define MAX_FIELDS      32
#define MAX_METHODS     32
#define MAX_LIST       512
#define MAX_CALL_DEPTH 256
#define MAX_ENV_VARS   256
#define SRC_MAX     (512*1024)

/* ════════════════════════════════════════════════════════════════════════
 * TOKENS
 * ════════════════════════════════════════════════════════════════════════ */
typedef enum {
    T_INT, T_FLOAT, T_STR, T_TRUE, T_FALSE, T_NONE,
    T_IDENT,
    T_LET, T_FIX, T_FN, T_CLASS, T_EXTENDS, T_USE,
    T_IF, T_ELSE, T_MATCH, T_FOR, T_WHILE, T_REPEAT, T_TIMES, T_IN,
    T_WAIT, T_RUN, T_SAY, T_LOG,
    T_ASK, T_INFER, T_FROM, T_FEELS, T_LIKE, T_ROUTE,
    T_AND, T_OR, T_NOT, T_IS,
    T_OK, T_FAIL, T_SOME,
    T_TEXT, T_TINT, T_TFLOAT, T_TBOOL, T_TLIST, T_TMAP, T_TOPTION, T_TRESULT,
    T_PLUS, T_MINUS, T_STAR, T_SLASH, T_PERCENT,
    T_EQEQ, T_NEQ, T_LT, T_LTEQ, T_GT, T_GTEQ,
    T_ARROW, T_QUESTION, T_DOT, T_PLUS_EQ, T_MINUS_EQ,
    T_EQ, T_COLON, T_COMMA,
    T_LP, T_RP, T_LB, T_RB, T_LC, T_RC,
    T_UNDER,
    T_NL, T_INDENT, T_DEDENT, T_EOF,
    T_COUNT
} TK;

typedef struct {
    TK   kind;
    int  line;
    union { i64 ival; double fval; } v;
    char *sval;        /* heap string for T_STR / T_IDENT */
    char  lex[MAX_IDENT];
} Token;

/* ════════════════════════════════════════════════════════════════════════
 * AST — lean pointer-based nodes
 * ════════════════════════════════════════════════════════════════════════ */
typedef enum {
    N_INT, N_FLOAT, N_STR, N_BOOL, N_NIL,
    N_IDENT,
    N_LIST, N_MAP,
    N_SOME, N_OK, N_FAIL,
    N_BINOP, N_UNARY,
    N_CALL, N_MCALL, N_FIELD, N_INDEX,
    N_LAMBDA,
    N_WAIT, N_PROP,          /* wait expr,  expr? */
    N_IN, N_IS,
    N_ASK, N_INFER,
    N_RANGE,
    N_LET, N_ASSIGN,
    N_FN, N_CLASS,
    N_IF, N_MATCH, N_ARM,
    N_FOR, N_WHILE, N_REPEAT,
    N_RET, N_SAY, N_LOG, N_RUN, N_USE,
    N_ROUTE, N_RARM,
    N_EXPR,                  /* expression statement */
    N_PROG                   /* top-level program */
} NK;

typedef enum { OP_ADD,OP_SUB,OP_MUL,OP_DIV,OP_MOD,
               OP_EQ,OP_NEQ,OP_LT,OP_LTEQ,OP_GT,OP_GTEQ,
               OP_AND,OP_OR,OP_NEG,OP_NOT } Op;

typedef struct Node Node;

/* A node list — dynamically grown */
typedef struct {
    Node **data;
    int    len;
    int    cap;
} NodeList;

/* String list */
typedef struct {
    char (*data)[MAX_IDENT];
    int   len;
    int   cap;
} StrList;

NodeList *nl_new(void);
void      nl_push(NodeList *l, Node *n);

struct Node {
    NK   kind;
    int  line;
    /* unified fields — only the relevant ones are used per node kind */
    i64    ival;
    double fval;
    int    bval;
    char  *sval;       /* heap-allocated string */
    Op     op;
    int    mutable;
    char   name[MAX_IDENT];   /* identifier, field, method, binding */
    Node  *a;          /* left / callee / object / value / condition */
    Node  *b;          /* right / index / iterable */
    Node  *c;          /* else branch */
    NodeList *kids;    /* body statements, match arms, list items */
    NodeList *kids2;   /* map values, route else */
    StrList  *params;  /* parameter names */
    StrList  *names2;  /* map keys (as strings), field names */
};

/* ════════════════════════════════════════════════════════════════════════
 * VALUES
 * ════════════════════════════════════════════════════════════════════════ */
typedef enum {
    V_NIL, V_BOOL, V_INT, V_FLOAT, V_TEXT,
    V_LIST, V_MAP,
    V_OPTION, V_RESULT,
    V_FN, V_CLASS, V_INST, V_NATIVE
} VK;

typedef struct Value Value;
typedef struct Env   Env;

typedef struct {
    char     name[MAX_IDENT];
    char   **params;
    int      paramc;
    NodeList *body;
    Env     *closure;
} VelFn;

typedef struct {
    char    name[MAX_IDENT];
    char  **fields;
    int     fieldc;
    VelFn *methods;
    int     methodc;
} VelClass;

typedef struct {
    char    class_name[MAX_IDENT];
    char  **fnames;
    Value **fvals;
    int     fieldc;
    VelFn *methods;
    int     methodc;
} VelInst;

struct Value {
    VK kind;
    union {
        int       bval;
        i64       ival;
        double    fval;
        char     *sval;
        struct { Value **items; int len; int cap; } list;
        struct { Value **keys; Value **vals; int len; } map;
        struct { int is_some; Value *inner; } opt;
        struct { int is_ok; Value *inner; char *err; } res;
        VelFn    fn;
        VelClass cls;
        VelInst  inst;
        char      nname[MAX_IDENT];
    };
};

/* ════════════════════════════════════════════════════════════════════════
 * ENVIRONMENT
 * ════════════════════════════════════════════════════════════════════════ */
typedef struct EnvSlot { char name[MAX_IDENT]; Value *val; int mut; } EnvSlot;
struct Env {
    EnvSlot  *slots;
    int       len;
    int       cap;
    Env      *parent;
};

/* ════════════════════════════════════════════════════════════════════════
 * INTERPRETER
 * ════════════════════════════════════════════════════════════════════════ */
#define SIG_NONE   0
#define SIG_RET    1
#define SIG_BRK    2

typedef struct {
    int    sig;
    Value *retval;
    char   err[MAX_STR];
    int    had_err;
    int    depth;
} Interp;

/* ════════════════════════════════════════════════════════════════════════
 * PROTOTYPES
 * ════════════════════════════════════════════════════════════════════════ */

/* lexer */
int    vel_lex(const char *src, Token *out, int max);

/* parser */
Node  *vel_parse(Token *toks, int n);
Node  *node_alloc(NK kind, int line);

/* interpreter */
void   interp_init(Interp *I);
Value *interp_run(Interp *I, Node *prog, Env *env);
Value *interp_eval(Interp *I, Node *n, Env *env);
void   interp_exec(Interp *I, Node *n, Env *env);
Value *call_fn(Interp *I, VelFn *fn, Value **args, int argc);

/* env */
Env   *env_new(Env *parent);
void   env_def(Env *e, const char *name, Value *v, int mut);
Value *env_get(Env *e, const char *name);
int    env_set(Env *e, const char *name, Value *v, Interp *I);

/* values */
Value *v_nil(void);
Value *v_bool(int b);
Value *v_int(i64 n);
Value *v_float(double f);
Value *v_text(const char *s);
Value *v_list(void);
Value *v_map(void);
Value *v_none(void);
Value *v_some(Value *x);
Value *v_ok(Value *x);
Value *v_fail(const char *msg);
Value *v_native(const char *name);
void   v_list_push(Value *list, Value *item);
int    v_truthy(Value *v);
int    v_equal(Value *a, Value *b);
char  *v_tostr(Value *v, char *buf, int sz);
void   v_print(Value *v, FILE *fp);
const char *v_typename(Value *v);

/* stdlib */
void   stdlib_register(Env *env);
Value *stdlib_call(const char *name, Value **args, int argc, Interp *I);
Value *stdlib_method(Value *obj, const char *meth, Value **args, int argc, Interp *I);

/* error */
void   vel_err(Interp *I, int line, const char *fmt, ...);
void   vel_fatal(const char *fmt, ...);

#endif
