/* parser.c — Recursive-descent parser for VEL. Pure C99, zero deps. */
#include "vel.h"

typedef struct { Token *t; int pos, n, err; char emsg[256]; } P;

static Token *pp(P *p)  { return &p->t[p->pos]; }
static TK    pk(P *p)   { return p->t[p->pos].kind; }
static Token *adv(P *p) { Token *t = &p->t[p->pos]; if (p->pos+1 < p->n) p->pos++; return t; }
static int   chk(P *p, TK k) { return pk(p) == k; }
static int   eat(P *p, TK k) { if (chk(p,k)){adv(p);return 1;} return 0; }
static Token *expect(P *p, TK k, const char *what) {
    if (!chk(p,k)) {
        snprintf(p->emsg, sizeof(p->emsg), "Expected %s at line %d, got '%s'", what, pp(p)->line, pp(p)->lex);
        p->err = 1;
    }
    return adv(p);
}
static void skip_nl(P *p) { while (chk(p, T_NL)) adv(p); }

static char *expect_ident(P *p) {
    if (pk(p) != T_IDENT) {
        snprintf(p->emsg, sizeof(p->emsg), "Expected identifier at line %d, got '%s'", pp(p)->line, pp(p)->lex);
        p->err = 1; return "";
    }
    Token *tok = adv(p);
    return tok->sval ? tok->sval : tok->lex;
}

/* Forward declarations */
static Node *parse_stmt(P *p);
static Node *parse_expr(P *p);

/* ── Node helpers ── */
static Node *mk(P *p, NK k) {
    Node *n = node_alloc(k, pp(p)->line);
    return n;
}
static Node *mk_at(NK k, int line) { return node_alloc(k, line); }

/* Ensure kids/kids2/params/names2 exist */
static NodeList *kids(Node *n) { if (!n->kids) n->kids = nl_new(); return n->kids; }
static NodeList *kids2(Node *n){ if (!n->kids2) n->kids2 = nl_new(); return n->kids2; }

static StrList *str_list_new(void) {
    StrList *l = calloc(1, sizeof(StrList));
    l->cap = 4; l->data = calloc(4, MAX_IDENT);
    return l;
}
static void str_push(StrList *l, const char *s) {
    if (l->len >= l->cap) { l->cap *= 2; l->data = realloc(l->data, l->cap * MAX_IDENT); }
    snprintf(l->data[l->len++], MAX_IDENT, "%s", s);
}
static StrList *params(Node *n) { if (!n->params) n->params = str_list_new(); return n->params; }
static StrList *names2(Node *n) { if (!n->names2) n->names2 = str_list_new(); return n->names2; }

/* ── Block ── */
static void parse_block(P *p, NodeList *body) {
    if (!eat(p, T_INDENT)) {
        Node *s = parse_stmt(p);
        if (s) nl_push(body, s);
        return;
    }
    skip_nl(p);
    while (!chk(p,T_DEDENT) && !chk(p,T_EOF) && !p->err) {
        Node *s = parse_stmt(p);
        if (s) nl_push(body, s);
        skip_nl(p);
    }
    eat(p, T_DEDENT);
}

/* ── Param list ── */
static void parse_params(P *p, StrList *plist) {
    expect(p, T_LP, "'('");
    while (!chk(p,T_RP) && !chk(p,T_EOF) && !p->err) {
        str_push(plist, expect_ident(p));
        /* consume optional type annotation */
        if (eat(p, T_COLON)) {
            /* skip type tokens */
            while (!chk(p,T_COMMA) && !chk(p,T_RP) && !chk(p,T_EOF))
                adv(p);
        }
        if (!eat(p, T_COMMA)) break;
    }
    expect(p, T_RP, "')'");
}

/* ── Statements ── */
static Node *parse_let(P *p, int mut) {
    adv(p);
    Node *n = mk(p, N_LET);
    n->mutable = mut;
    snprintf(n->name, MAX_IDENT, "%s", expect_ident(p));
    if (eat(p, T_COLON))
        while (!chk(p,T_EQ) && !chk(p,T_NL) && !chk(p,T_EOF)) adv(p); /* skip type */
    expect(p, T_EQ, "'='");
    n->a = parse_expr(p);
    eat(p, T_NL);
    return n;
}

/* Skip a complete type annotation including optional [Inner] brackets */
static void skip_type(P *p) {
    /* consume the base type keyword or ident */
    adv(p);
    /* consume optional [Inner] or [K, V] */
    if (eat(p, T_LB)) {
        int depth = 1;
        while (depth > 0 && !chk(p, T_EOF)) {
            if      (chk(p, T_LB)) depth++;
            else if (chk(p, T_RB)) depth--;
            adv(p);
        }
    }
}

static int is_type_start(P *p) {
    TK k = pk(p);
    return k==T_TEXT||k==T_TINT||k==T_TFLOAT||k==T_TBOOL||
           k==T_TLIST||k==T_TMAP||k==T_TOPTION||k==T_TRESULT;
}

static Node *parse_fn(P *p) {
    int line = pp(p)->line;
    adv(p); /* fn */
    Node *n = mk_at(N_FN, line);
    snprintf(n->name, MAX_IDENT, "%s", expect_ident(p));
    parse_params(p, params(n));
    n->kids = nl_new();

    /* optional return type annotation, then body */
    if (eat(p, T_ARROW)) {
        if (is_type_start(p)) {
            skip_type(p); /* skip return type (may include [T]) */
            if (eat(p, T_COLON)) { eat(p,T_NL); parse_block(p, n->kids); }
            /* else: no body — forward declaration, ignore */
        } else {
            /* one-liner: fn foo(x) -> expr */
            Node *ret = mk_at(N_RET, line);
            ret->a = parse_expr(p);
            nl_push(n->kids, ret);
            eat(p, T_NL);
        }
    } else if (eat(p, T_COLON)) {
        eat(p, T_NL);
        parse_block(p, n->kids);
    }
    return n;
}

static Node *parse_class(P *p) {
    adv(p);
    Node *n = mk(p, N_CLASS);
    snprintf(n->name, MAX_IDENT, "%s", expect_ident(p));
    if (eat(p, T_EXTENDS)) {
        /* store super name in sval */
        n->sval = strdup(expect_ident(p));
    }
    expect(p, T_COLON, "':'");
    eat(p, T_NL);
    expect(p, T_INDENT, "class body");
    n->kids  = nl_new();  /* methods */
    n->names2= str_list_new(); /* field names */
    skip_nl(p);
    while (!chk(p,T_DEDENT)&&!chk(p,T_EOF)&&!p->err) {
        if (chk(p,T_FN)) nl_push(n->kids, parse_fn(p));
        else {
            str_push(names2(n), expect_ident(p));
            if (eat(p,T_COLON)) while(!chk(p,T_NL)&&!chk(p,T_EOF)) adv(p);
            eat(p,T_NL);
        }
        skip_nl(p);
    }
    eat(p, T_DEDENT);
    return n;
}

static Node *parse_if(P *p) {
    adv(p);
    Node *n = mk(p, N_IF);
    n->a = parse_expr(p);
    expect(p, T_COLON, "':'"); eat(p, T_NL);
    n->kids = nl_new();
    parse_block(p, n->kids);
    skip_nl(p);
    if (eat(p, T_ELSE)) {
        n->kids2 = nl_new();
        if (chk(p, T_IF)) { nl_push(n->kids2, parse_if(p)); }
        else { expect(p,T_COLON,"':'"); eat(p,T_NL); parse_block(p,n->kids2); }
    }
    return n;
}

static Node *parse_pattern(P *p) {
    switch (pk(p)) {
        case T_UNDER: { adv(p); Node *n = node_alloc(N_IDENT, pp(p)->line); strcpy(n->name,"_"); return n; }
        case T_NONE:  { adv(p); return node_alloc(N_NIL, pp(p)->line); }
        case T_OK:    { adv(p); Node *n = node_alloc(N_OK,   pp(p)->line);
                        if(eat(p,T_LP)){strncpy(n->name,expect_ident(p),MAX_IDENT-1);eat(p,T_RP);} return n; }
        case T_FAIL:  { adv(p); Node *n = node_alloc(N_FAIL, pp(p)->line);
                        if(eat(p,T_LP)){strncpy(n->name,expect_ident(p),MAX_IDENT-1);eat(p,T_RP);} return n; }
        case T_SOME:  { adv(p); Node *n = node_alloc(N_SOME, pp(p)->line);
                        if(eat(p,T_LP)){strncpy(n->name,expect_ident(p),MAX_IDENT-1);eat(p,T_RP);} return n; }
        case T_IDENT: {
            /* peek ahead: if followed by an operator, treat as full expression */
            int nxt = p->t[p->pos + 1].kind;
            if (nxt == T_PERCENT || nxt == T_PLUS || nxt == T_MINUS ||
                nxt == T_STAR   || nxt == T_SLASH ||
                nxt == T_EQEQ  || nxt == T_NEQ   ||
                nxt == T_LT    || nxt == T_LTEQ  ||
                nxt == T_GT    || nxt == T_GTEQ  ||
                nxt == T_AND   || nxt == T_OR    ||
                nxt == T_DOT   || nxt == T_LP    ||
                nxt == T_LB) {
                return parse_expr(p);
            }
            Node *n = node_alloc(N_IDENT, pp(p)->line);
            Token *tok = adv(p);
            snprintf(n->name, MAX_IDENT, "%s", tok->sval ? tok->sval : tok->lex);
            return n;
        }
        default:      return parse_expr(p);
    }
}

static Node *parse_match(P *p) {
    adv(p);
    Node *n = mk(p, N_MATCH);
    n->a = parse_expr(p);
    expect(p, T_COLON, "':'"); eat(p,T_NL);
    expect(p, T_INDENT, "match body");
    n->kids = nl_new();
    skip_nl(p);
    while (!chk(p,T_DEDENT)&&!chk(p,T_EOF)&&!p->err) {
        Node *arm = mk(p, N_ARM);
        arm->a = parse_pattern(p);
        expect(p, T_ARROW, "'->'");
        /* arm body: a say/log is a statement; otherwise expression */
        if (chk(p,T_SAY)) {
            adv(p);
            Node *say = mk_at(N_SAY, pp(p)->line); say->a = parse_expr(p);
            arm->b = say;
        } else if (chk(p,T_LOG)) {
            adv(p);
            Node *log = mk_at(N_LOG, pp(p)->line); log->a = parse_expr(p);
            arm->b = log;
        } else {
            arm->b = parse_expr(p);
        }
        eat(p,T_NL); skip_nl(p);
        nl_push(n->kids, arm);
    }
    eat(p,T_DEDENT);
    return n;
}

static Node *parse_for(P *p) {
    adv(p);
    Node *n = mk(p, N_FOR);
    snprintf(n->name, MAX_IDENT, "%s", expect_ident(p));
    expect(p, T_IN, "'in'");
    n->a = parse_expr(p);
    expect(p, T_COLON, "':'"); eat(p,T_NL);
    n->kids = nl_new();
    parse_block(p, n->kids);
    return n;
}

static Node *parse_while(P *p) {
    adv(p);
    Node *n = mk(p, N_WHILE);
    n->a = parse_expr(p);
    expect(p, T_COLON, "':'"); eat(p,T_NL);
    n->kids = nl_new();
    parse_block(p, n->kids);
    return n;
}

static Node *parse_repeat(P *p) {
    adv(p);
    Node *n = mk(p, N_REPEAT);
    n->a = parse_expr(p);
    expect(p, T_TIMES, "'times'");
    expect(p, T_COLON, "':'"); eat(p,T_NL);
    n->kids = nl_new();
    parse_block(p, n->kids);
    return n;
}

static Node *parse_route(P *p) {
    adv(p);
    Node *n = mk(p, N_ROUTE);
    n->a = parse_expr(p);
    expect(p, T_COLON, "':'"); eat(p,T_NL);
    expect(p, T_INDENT, "route body");
    n->kids  = nl_new();
    skip_nl(p);
    while (!chk(p,T_DEDENT)&&!chk(p,T_EOF)&&!p->err) {
        if (eat(p, T_IF)) {
            Node *arm = mk(p, N_RARM);
            expect(p, T_FEELS, "'feels'"); expect(p, T_LIKE, "'like'");
            arm->a = parse_expr(p);
            expect(p, T_ARROW, "'->'");
            /* allow say/log as handler */
            if (chk(p,T_SAY)) { adv(p); Node *s=mk_at(N_SAY,pp(p)->line); s->a=parse_expr(p); arm->b=s; }
            else if (chk(p,T_LOG)) { adv(p); Node *s=mk_at(N_LOG,pp(p)->line); s->a=parse_expr(p); arm->b=s; }
            else { arm->b = parse_expr(p); }
            nl_push(n->kids, arm);
        } else if (eat(p, T_ELSE)) {
            expect(p, T_ARROW, "'->'");
            /* allow say/log as else handler */
            if (chk(p,T_SAY)) { adv(p); Node *s=mk_at(N_SAY,pp(p)->line); s->a=parse_expr(p); n->b=s; }
            else if (chk(p,T_LOG)) { adv(p); Node *s=mk_at(N_LOG,pp(p)->line); s->a=parse_expr(p); n->b=s; }
            else { n->b = parse_expr(p); }
        }
        eat(p,T_NL); skip_nl(p);
    }
    eat(p, T_DEDENT);
    return n;
}

static Node *parse_stmt(P *p) {
    skip_nl(p);
    if (p->err || chk(p,T_EOF)) return NULL;
    int line = pp(p)->line;

    switch (pk(p)) {
        case T_LET:    return parse_let(p, 1);
        case T_FIX:    return parse_let(p, 0);
        case T_FN:     return parse_fn(p);
        case T_CLASS:  return parse_class(p);
        case T_IF:     return parse_if(p);
        case T_MATCH:  return parse_match(p);
        case T_FOR:    return parse_for(p);
        case T_WHILE:  return parse_while(p);
        case T_REPEAT: return parse_repeat(p);
        case T_ROUTE:  return parse_route(p);
        case T_SAY:  { adv(p); Node *n=mk_at(N_SAY,line); n->a=parse_expr(p); eat(p,T_NL); return n; }
        case T_LOG:  { adv(p); Node *n=mk_at(N_LOG,line); n->a=parse_expr(p); eat(p,T_NL); return n; }
        case T_RUN:  { adv(p); Node *n=mk_at(N_RUN,line); n->a=parse_expr(p); eat(p,T_NL); return n; }
        case T_USE:  { adv(p); Node *n=mk_at(N_USE,line);
                       char path[MAX_IDENT]=""; strncpy(path,expect_ident(p),MAX_IDENT-1);
                       while(eat(p,T_DOT)){strncat(path,".",MAX_IDENT-strlen(path)-1); strncat(path,expect_ident(p),MAX_IDENT-strlen(path)-1);}
                       n->sval = strdup(path); eat(p,T_NL); return n; }
        case T_ARROW:{ adv(p); Node *n=mk_at(N_RET,line);
                       if(!chk(p,T_NL)&&!chk(p,T_EOF)) n->a=parse_expr(p);
                       eat(p,T_NL); return n; }
        default: {
            Node *e = parse_expr(p);
            if (chk(p,T_EQ)||chk(p,T_PLUS_EQ)||chk(p,T_MINUS_EQ)) {
                int op = chk(p,T_EQ)?0:chk(p,T_PLUS_EQ)?1:2;
                adv(p);
                Node *n = mk_at(N_ASSIGN, line);
                n->a = e; n->b = parse_expr(p); n->bval = op;
                eat(p,T_NL); return n;
            }
            eat(p, T_NL);
            Node *n = mk_at(N_EXPR, line); n->a = e; return n;
        }
    }
}

/* ── Expressions ── */
static Node *parse_or(P *p);
static Node *parse_and(P *p);
static Node *parse_not(P *p);
static Node *parse_cmp(P *p);
static Node *parse_add(P *p);
static Node *parse_mul(P *p);
static Node *parse_unary(P *p);
static Node *parse_post(P *p);
static Node *parse_prim(P *p);

static Node *parse_expr(P *p) { return parse_or(p); }

static Node *parse_or(P *p) {
    Node *l = parse_and(p);
    while (!p->err && chk(p,T_OR)) {
        int line = pp(p)->line; adv(p);
        Node *n = mk_at(N_BINOP,line); n->op=OP_OR; n->a=l; n->b=parse_and(p); l=n;
    }
    return l;
}
static Node *parse_and(P *p) {
    Node *l = parse_not(p);
    while (!p->err && chk(p,T_AND)) {
        int line = pp(p)->line; adv(p);
        Node *n = mk_at(N_BINOP,line); n->op=OP_AND; n->a=l; n->b=parse_not(p); l=n;
    }
    return l;
}
static Node *parse_not(P *p) {
    if (eat(p,T_NOT)) {
        int line = pp(p)->line;
        Node *n = mk_at(N_UNARY,line); n->op=OP_NOT; n->a=parse_not(p); return n;
    }
    return parse_cmp(p);
}
static Node *parse_cmp(P *p) {
    Node *l = parse_add(p);
    while (!p->err) {
        int line = pp(p)->line;
        Op op; int do_in=0, do_is=0;
        switch(pk(p)){
            case T_EQEQ: op=OP_EQ;   break; case T_NEQ:  op=OP_NEQ;  break;
            case T_LT:   op=OP_LT;   break; case T_LTEQ: op=OP_LTEQ; break;
            case T_GT:   op=OP_GT;   break; case T_GTEQ: op=OP_GTEQ; break;
            case T_IN:   do_in=1; op=OP_EQ; break;
            case T_IS:   do_is=1; op=OP_EQ; break;
            default: goto done_cmp;
        }
        adv(p);
        if (do_in) { Node *n=mk_at(N_IN,line); n->a=l; n->b=parse_add(p); l=n; }
        else if (do_is) {
            Node *n=mk_at(N_IS,line); n->a=l;
            snprintf(n->name, MAX_IDENT, "%s", pp(p)->lex); adv(p); l=n;
        }
        else { Node *n=mk_at(N_BINOP,line); n->op=op; n->a=l; n->b=parse_add(p); l=n; }
    }
    done_cmp: return l;
}
static Node *parse_add(P *p) {
    Node *l = parse_mul(p);
    while (!p->err && (chk(p,T_PLUS)||chk(p,T_MINUS))) {
        int line=pp(p)->line; Op op=chk(p,T_PLUS)?OP_ADD:OP_SUB; adv(p);
        Node *n=mk_at(N_BINOP,line); n->op=op; n->a=l; n->b=parse_mul(p); l=n;
    }
    return l;
}
static Node *parse_mul(P *p) {
    Node *l = parse_unary(p);
    while (!p->err && (chk(p,T_STAR)||chk(p,T_SLASH)||chk(p,T_PERCENT))) {
        int line=pp(p)->line; Op op=chk(p,T_STAR)?OP_MUL:chk(p,T_SLASH)?OP_DIV:OP_MOD; adv(p);
        Node *n=mk_at(N_BINOP,line); n->op=op; n->a=l; n->b=parse_unary(p); l=n;
    }
    return l;
}
static Node *parse_unary(P *p) {
    if (chk(p,T_MINUS)) {
        int line=pp(p)->line; adv(p);
        Node *n=mk_at(N_UNARY,line); n->op=OP_NEG; n->a=parse_unary(p); return n;
    }
    return parse_post(p);
}

static void fill_args(P *p, NodeList *args) {
    expect(p, T_LP, "'('");
    while (!chk(p,T_RP)&&!chk(p,T_EOF)&&!p->err) {
        /* keyword argument: ident: expr  — skip the label, just parse the value */
        if (pk(p)==T_IDENT && p->t[p->pos+1].kind==T_COLON) {
            adv(p); adv(p); /* skip name and colon */
        }
        nl_push(args, parse_expr(p));
        if (!eat(p,T_COMMA)) break;
    }
    expect(p, T_RP, "')'");
}

static Node *parse_post(P *p) {
    Node *e = parse_prim(p);
    while (!p->err) {
        /* allow indented method chain continuation across newlines/indents/dedents */
        while (chk(p,T_NL)||chk(p,T_INDENT)||chk(p,T_DEDENT)) {
            int look = p->pos + 1;
            while (look < p->n &&
                   (p->t[look].kind==T_NL ||
                    p->t[look].kind==T_INDENT ||
                    p->t[look].kind==T_DEDENT))
                look++;
            if (p->t[look].kind == T_DOT) { adv(p); }
            else break;
        }

        if (chk(p,T_DOT)) {
            int line=pp(p)->line; adv(p);
            char mname[MAX_IDENT];
            snprintf(mname, MAX_IDENT, "%s", pp(p)->lex); adv(p);
            if (chk(p,T_LP)) {
                Node *n=mk_at(N_MCALL,line); n->a=e;
                snprintf(n->name, MAX_IDENT, "%s", mname);
                n->kids=nl_new(); fill_args(p, n->kids); e=n;
            } else {
                Node *n=mk_at(N_FIELD,line); n->a=e;
                snprintf(n->name, MAX_IDENT, "%s", mname); e=n;
            }
        } else if (chk(p,T_LB)) {
            int line=pp(p)->line; adv(p);
            Node *n=mk_at(N_INDEX,line); n->a=e; n->b=parse_expr(p);
            expect(p,T_RB,"']'"); e=n;
        } else if (chk(p,T_QUESTION)) {
            int line=pp(p)->line; adv(p);
            Node *n=mk_at(N_PROP,line); n->a=e; e=n;
        } else break;
    }
    return e;
}

static Node *parse_prim(P *p) {
    int line = pp(p)->line;
    switch (pk(p)) {
        case T_INT:   { Node *n=mk_at(N_INT,line);   n->ival=adv(p)->v.ival; return n; }
        case T_FLOAT: { Node *n=mk_at(N_FLOAT,line); n->fval=adv(p)->v.fval; return n; }
        case T_STR: {
            Token *tok = adv(p);
            Node *n = mk_at(N_STR, line);
            n->sval = tok->sval ? strdup(tok->sval) : strdup("");
            return n;
        }
        case T_TRUE:  { adv(p); Node *n=mk_at(N_BOOL,line);  n->bval=1; return n; }
        case T_FALSE: { adv(p); Node *n=mk_at(N_BOOL,line);  n->bval=0; return n; }
        case T_NONE:  { adv(p); return mk_at(N_NIL,line); }
        case T_SOME:  { adv(p); Node *n=mk_at(N_SOME,line); expect(p,T_LP,"'('"); n->a=parse_expr(p); expect(p,T_RP,"')'"); return n; }
        case T_OK:    { adv(p); Node *n=mk_at(N_OK,line);   expect(p,T_LP,"'('"); n->a=parse_expr(p); expect(p,T_RP,"')'"); return n; }
        case T_FAIL:  { adv(p); Node *n=mk_at(N_FAIL,line); expect(p,T_LP,"'('"); n->a=parse_expr(p); expect(p,T_RP,"')'"); return n; }
        case T_WAIT:  { adv(p); Node *n=mk_at(N_WAIT,line); n->a=parse_expr(p); return n; }
        /* Inline if expression: if cond: then else: else_val */
        case T_IF: {
            adv(p);
            Node *n=mk_at(N_IF,line);
            n->a = parse_expr(p);           /* condition */
            expect(p,T_COLON,"':'");
            /* then: single expression on same line */
            Node *then_expr = parse_expr(p);
            Node *then_ret  = mk_at(N_RET,line); then_ret->a = then_expr;
            n->kids = nl_new(); nl_push(n->kids, then_ret);
            /* optional else */
            if (eat(p,T_ELSE)) {
                expect(p,T_COLON,"':'");
                Node *else_expr = parse_expr(p);
                Node *else_ret  = mk_at(N_RET,line); else_ret->a = else_expr;
                n->kids2 = nl_new(); nl_push(n->kids2, else_ret);
            }
            return n;
        }
        case T_ASK:   { adv(p); Node *n=mk_at(N_ASK,line); n->a=parse_prim(p); expect(p,T_COLON,"':'"); n->b=parse_expr(p); return n; }
        case T_INFER: { adv(p); Node *n=mk_at(N_INFER,line); expect(p,T_FROM,"'from'"); n->a=parse_prim(p); expect(p,T_COLON,"':'"); n->b=parse_expr(p); return n; }
        case T_FN: {
            adv(p);
            Node *n=mk_at(N_LAMBDA,line);
            n->params=str_list_new(); parse_params(p, n->params);
            expect(p,T_ARROW,"'->'"); n->a=parse_expr(p); return n;
        }
        case T_LB: {
            adv(p); Node *n=mk_at(N_LIST,line); n->kids=nl_new();
            while(chk(p,T_NL)||chk(p,T_INDENT)||chk(p,T_DEDENT)) adv(p);
            while (!chk(p,T_RB)&&!chk(p,T_EOF)&&!p->err) {
                nl_push(n->kids,parse_expr(p));
                eat(p,T_COMMA);
                while(chk(p,T_NL)||chk(p,T_INDENT)||chk(p,T_DEDENT)) adv(p);
            }
            expect(p,T_RB,"']'"); return n;
        }
        case T_LC: {
            adv(p); Node *n=mk_at(N_MAP,line); n->kids=nl_new(); n->names2=str_list_new();
            /* skip whitespace at start of map body */
            while(chk(p,T_NL)||chk(p,T_INDENT)||chk(p,T_DEDENT)) adv(p);
            while (!chk(p,T_RC)&&!chk(p,T_EOF)&&!p->err) {
                /* key: ident or string literal */
                char kname[MAX_IDENT] = "";
                if (pk(p)==T_IDENT) { snprintf(kname, MAX_IDENT, "%s", pp(p)->sval ? pp(p)->sval : pp(p)->lex); adv(p); }
                else if (pk(p)==T_STR) { snprintf(kname, MAX_IDENT, "%s", pp(p)->sval ? pp(p)->sval : ""); adv(p); }
                else { snprintf(kname, MAX_IDENT, "%s", pp(p)->lex); adv(p); }
                str_push(names2(n), kname);
                expect(p,T_COLON,"':'");
                nl_push(n->kids, parse_expr(p));
                eat(p,T_COMMA);
                /* skip whitespace between entries */
                while(chk(p,T_NL)||chk(p,T_INDENT)||chk(p,T_DEDENT)) adv(p);
            }
            expect(p,T_RC,"'}'"); return n;
        }
        case T_LP: {
            adv(p); Node *inner=parse_expr(p); expect(p,T_RP,"')'"); return inner;
        }
        case T_IDENT: {
            Token *tok = adv(p);
            Node *n = mk_at(N_IDENT, line);
            snprintf(n->name, MAX_IDENT, "%s", tok->sval ? tok->sval : tok->lex);
            if (chk(p, T_LP)) {
                Node *call = mk_at(N_CALL, line);
                call->a = n; call->kids = nl_new();
                fill_args(p, call->kids); return call;
            }
            return n;
        }
        default: {
            snprintf(p->emsg,sizeof(p->emsg),"Unexpected token '%s' at line %d", pp(p)->lex, line);
            p->err=1; return mk_at(N_NIL,line);
        }
    }
}

/* ── Entry ── */
Node *vel_parse(Token *toks, int n) {
    P p = {0}; p.t = toks; p.n = n;
    Node *prog = node_alloc(N_PROG, 1);
    prog->kids = nl_new();
    /* skip_top: eat NL, INDENT, DEDENT at top level */
    #define skip_top(pp) do { \
        while(chk(pp,T_NL)||chk(pp,T_INDENT)||chk(pp,T_DEDENT)) adv(pp); \
    } while(0)
    skip_top(&p);
    while (!chk(&p,T_EOF) && !p.err) {
        Node *s = parse_stmt(&p);
        if (s) nl_push(prog->kids, s);
        skip_top(&p);
    }
    #undef skip_top
    if (p.err) { fprintf(stderr, "\033[31mparse error:\033[0m %s\n", p.emsg); return NULL; }
    return prog;
}
