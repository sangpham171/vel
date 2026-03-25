/* lexer.c — VEL tokenizer. Pure C99, zero deps. */
#include "vel.h"

/* Keyword lookup defined inline below */
static TK kw_kind(const char *w) {
    if (!strcmp(w,"let"))     return T_LET;
    if (!strcmp(w,"fix"))     return T_FIX;
    if (!strcmp(w,"fn"))      return T_FN;
    if (!strcmp(w,"class"))   return T_CLASS;
    if (!strcmp(w,"extends")) return T_EXTENDS;
    if (!strcmp(w,"use"))     return T_USE;
    if (!strcmp(w,"if"))      return T_IF;
    if (!strcmp(w,"else"))    return T_ELSE;
    if (!strcmp(w,"match"))   return T_MATCH;
    if (!strcmp(w,"for"))     return T_FOR;
    if (!strcmp(w,"while"))   return T_WHILE;
    if (!strcmp(w,"repeat"))  return T_REPEAT;
    if (!strcmp(w,"times"))   return T_TIMES;
    if (!strcmp(w,"in"))      return T_IN;
    if (!strcmp(w,"wait"))    return T_WAIT;
    if (!strcmp(w,"run"))     return T_RUN;
    if (!strcmp(w,"say"))     return T_SAY;
    if (!strcmp(w,"log"))     return T_LOG;
    if (!strcmp(w,"ask"))     return T_ASK;
    if (!strcmp(w,"infer"))   return T_INFER;
    if (!strcmp(w,"from"))    return T_FROM;
    if (!strcmp(w,"feels"))   return T_FEELS;
    if (!strcmp(w,"like"))    return T_LIKE;
    if (!strcmp(w,"route"))   return T_ROUTE;
    if (!strcmp(w,"and"))     return T_AND;
    if (!strcmp(w,"or"))      return T_OR;
    if (!strcmp(w,"not"))     return T_NOT;
    if (!strcmp(w,"is"))      return T_IS;
    if (!strcmp(w,"ok"))      return T_OK;
    if (!strcmp(w,"fail"))    return T_FAIL;
    if (!strcmp(w,"some"))    return T_SOME;
    if (!strcmp(w,"none"))    return T_NONE;
    if (!strcmp(w,"true"))    return T_TRUE;
    if (!strcmp(w,"false"))   return T_FALSE;
    if (!strcmp(w,"Text"))    return T_TEXT;
    if (!strcmp(w,"Int"))     return T_TINT;
    if (!strcmp(w,"Float"))   return T_TFLOAT;
    if (!strcmp(w,"Bool"))    return T_TBOOL;
    if (!strcmp(w,"List"))    return T_TLIST;
    if (!strcmp(w,"Map"))     return T_TMAP;
    if (!strcmp(w,"Option"))  return T_TOPTION;
    if (!strcmp(w,"Result"))  return T_TRESULT;
    return T_IDENT;
}

typedef struct {
    const char *src;
    int pos, len, line, col;
    int indent_stack[256];
    int itop;
    Token *out;
    int ocount, omax;
    int at_line_start;
    int had_err;
    char err[256];
} Lx;

static void lx_err(Lx *L, const char *msg) {
    snprintf(L->err, sizeof(L->err), "%s (line %d)", msg, L->line);
    L->had_err = 1;
}
static char lpeek(Lx *L)  { return L->pos < L->len ? L->src[L->pos] : 0; }
static char lpeek2(Lx *L) { return L->pos+1 < L->len ? L->src[L->pos+1] : 0; }
static char ladv(Lx *L) {
    char c = L->src[L->pos++];
    if (c == '\n') { L->line++; L->col = 1; } else L->col++;
    return c;
}

static void push_tok(Lx *L, TK k, const char *lex, char *sval, i64 iv, double fv) {
    if (L->ocount >= L->omax) { lx_err(L, "Token limit exceeded"); return; }
    Token *t = &L->out[L->ocount++];
    memset(t, 0, sizeof(*t));
    t->kind = k; t->line = L->line;
    t->sval = sval;
    /* Write only the relevant union field — they share the same memory */
    if (k == T_FLOAT) t->v.fval = fv;
    else              t->v.ival = iv;
    if (lex) snprintf(t->lex, MAX_IDENT, "%s", lex);
}

static void do_indent(Lx *L) {
    int sp = 0;
    while (lpeek(L) == ' ')  { sp += 1; ladv(L); }
    while (lpeek(L) == '\t') { sp += 4; ladv(L); }
    char p = lpeek(L);
    if (p == '\n' || p == '\r' || p == '\0') return;
    if (p == '/' && lpeek2(L) == '/') return;
    int cur = L->indent_stack[L->itop];
    if (sp > cur) {
        L->indent_stack[++L->itop] = sp;
        push_tok(L, T_INDENT, "<indent>", NULL, 0, 0);
    } else if (sp < cur) {
        while (sp < L->indent_stack[L->itop]) {
            L->itop--;
            push_tok(L, T_DEDENT, "<dedent>", NULL, 0, 0);
        }
        if (sp != L->indent_stack[L->itop])
            lx_err(L, "Inconsistent indentation");
    }
}

static void lex_str(Lx *L) {
    ladv(L); /* consume " */
    char *buf = malloc(MAX_STR);
    int bi = 0;
    while (lpeek(L) != '"') {
        if (!lpeek(L) || lpeek(L) == '\n') { lx_err(L, "Unterminated string"); free(buf); return; }
        char c = ladv(L);
        if (c == '\\') {
            char e = ladv(L);
            switch(e) { case 'n': buf[bi++]='\n'; break; case 't': buf[bi++]='\t'; break;
                        case '"': buf[bi++]='"';  break; case '\\': buf[bi++]='\\'; break;
                        default:  buf[bi++]='\\'; buf[bi++]=e; break; }
        } else buf[bi++] = c;
        if (bi >= MAX_STR-1) { lx_err(L, "String too long"); free(buf); return; }
    }
    buf[bi] = '\0';
    ladv(L); /* consume closing " */
    push_tok(L, T_STR, buf, buf, 0, 0);
}

static void lex_num(Lx *L) {
    char buf[64]; int bi = 0; int is_f = 0;
    while (isdigit((u8)lpeek(L))) buf[bi++] = ladv(L);
    if (lpeek(L) == '.' && isdigit((u8)lpeek2(L))) {
        is_f = 1; buf[bi++] = ladv(L);
        while (isdigit((u8)lpeek(L))) buf[bi++] = ladv(L);
    }
    buf[bi] = '\0';
    if (is_f) push_tok(L, T_FLOAT, buf, NULL, 0, atof(buf));
    else      push_tok(L, T_INT,   buf, NULL, atoll(buf), 0);
}

static void lex_ident(Lx *L) {
    char buf[MAX_IDENT]; int bi = 0;
    while (isalnum((u8)lpeek(L)) || lpeek(L)=='_') buf[bi++] = ladv(L);
    buf[bi] = '\0';
    TK k = kw_kind(buf);
    char *sv = (k == T_IDENT) ? strdup(buf) : NULL;
    i64 iv = 0;
    if (k == T_TRUE)  iv = 1;
    if (k == T_FALSE) iv = 0;
    push_tok(L, k, buf, sv, iv, 0);
}

static void lex_sym(Lx *L) {
    char c = ladv(L), n = lpeek(L);
    switch(c) {
        case '+': if(n=='='){ladv(L);push_tok(L,T_PLUS_EQ, "+=",NULL,0,0);}
                  else {push_tok(L,T_PLUS,"+",NULL,0,0);} break;
        case '-': if(n=='>'){ladv(L);push_tok(L,T_ARROW,"->",NULL,0,0);}
                  else if(n=='='){ladv(L);push_tok(L,T_MINUS_EQ,"-=",NULL,0,0);}
                  else {push_tok(L,T_MINUS,"-",NULL,0,0);} break;
        case '*': push_tok(L,T_STAR,"*",NULL,0,0); break;
        case '/': push_tok(L,T_SLASH,"/",NULL,0,0); break;
        case '%': push_tok(L,T_PERCENT,"%",NULL,0,0); break;
        case '=': if(n=='='){ladv(L);push_tok(L,T_EQEQ,"==",NULL,0,0);}
                  else {push_tok(L,T_EQ,"=",NULL,0,0);} break;
        case '!': if(n=='='){ladv(L);push_tok(L,T_NEQ,"!=",NULL,0,0);}
                  else {lx_err(L,"Unexpected '!'");} break;
        case '<': if(n=='='){ladv(L);push_tok(L,T_LTEQ,"<=",NULL,0,0);}
                  else {push_tok(L,T_LT,"<",NULL,0,0);} break;
        case '>': if(n=='='){ladv(L);push_tok(L,T_GTEQ,">=",NULL,0,0);}
                  else {push_tok(L,T_GT,">",NULL,0,0);} break;
        case ':': push_tok(L,T_COLON,":",NULL,0,0); break;
        case ',': push_tok(L,T_COMMA,",",NULL,0,0); break;
        case '(': push_tok(L,T_LP,"(",NULL,0,0); break;
        case ')': push_tok(L,T_RP,")",NULL,0,0); break;
        case '[': push_tok(L,T_LB,"[",NULL,0,0); break;
        case ']': push_tok(L,T_RB,"]",NULL,0,0); break;
        case '{': push_tok(L,T_LC,"{",NULL,0,0); break;
        case '}': push_tok(L,T_RC,"}",NULL,0,0); break;
        case '?': push_tok(L,T_QUESTION,"?",NULL,0,0); break;
        case '_': push_tok(L,T_UNDER,"_",NULL,0,0); break;
        case '.': push_tok(L,T_DOT,".",NULL,0,0); break;
        default: { char msg[32]; snprintf(msg,sizeof(msg),"Unexpected char '%c'",c); lx_err(L,msg); }
    }
}

int vel_lex(const char *src, Token *out, int max) {
    Lx L = {0};
    L.src = src; L.len = (int)strlen(src);
    L.line = 1; L.col = 1;
    L.out = out; L.omax = max;
    L.at_line_start = 1;

    while (L.pos < L.len && !L.had_err) {
        if (L.at_line_start) { L.at_line_start = 0; do_indent(&L); }
        char c = lpeek(&L);
        if (!c) break;
        if (c == '\n') {
            ladv(&L);
            if (L.ocount && L.out[L.ocount-1].kind != T_NL)
                push_tok(&L, T_NL, "\\n", NULL, 0, 0);
            L.at_line_start = 1;
        } else if (c == '\r') { ladv(&L); }
        else if (c == ' ' || c == '\t') { ladv(&L); }
        else if (c == '/' && lpeek2(&L) == '/') {
            while (lpeek(&L) && lpeek(&L) != '\n') ladv(&L);
        }
        else if (c == '"')              lex_str(&L);
        else if (isdigit((u8)c))        lex_num(&L);
        else if (isalpha((u8)c)||c=='_') lex_ident(&L);
        else                            lex_sym(&L);
    }
    while (L.itop > 0) { L.itop--; push_tok(&L, T_DEDENT, "<dedent>", NULL, 0, 0); }
    push_tok(&L, T_EOF, "", NULL, 0, 0);
    if (L.had_err) { fprintf(stderr, "\033[31mlex error:\033[0m %s\n", L.err); return -1; }
    return L.ocount;
}
