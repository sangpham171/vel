/* util.c — Dynamic lists for AST nodes and strings */
#include "vel.h"

NodeList *nl_new(void) {
    NodeList *l = calloc(1, sizeof(NodeList));
    l->cap  = 4;
    l->data = calloc(4, sizeof(Node*));
    return l;
}
void nl_push(NodeList *l, Node *n) {
    if (l->len >= l->cap) {
        l->cap *= 2;
        l->data = realloc(l->data, l->cap * sizeof(Node*));
    }
    l->data[l->len++] = n;
}

static StrList *sl_new(void) {
    StrList *l = calloc(1, sizeof(StrList));
    l->cap  = 4;
    l->data = calloc(4, MAX_IDENT);
    return l;
}
static void sl_push(StrList *l, const char *s) {
    if (l->len >= l->cap) {
        l->cap *= 2;
        l->data = realloc(l->data, l->cap * MAX_IDENT);
    }
    strncpy(l->data[l->len++], s, MAX_IDENT - 1);
}

/* Node allocation */
Node *node_alloc(NK kind, int line) {
    Node *n = calloc(1, sizeof(Node));
    n->kind = kind;
    n->line = line;
    return n;
}
