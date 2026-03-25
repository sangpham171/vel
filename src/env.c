/* env.c — Lexical environment */
#include "vel.h"

Env *env_new(Env *parent) {
    Env *e = calloc(1, sizeof(Env));
    e->parent = parent;
    e->cap    = 16;
    e->slots  = calloc(16, sizeof(EnvSlot));
    return e;
}

void env_def(Env *e, const char *name, Value *v, int mut) {
    for (int i = 0; i < e->len; i++) {
        if (!strcmp(e->slots[i].name, name)) {
            e->slots[i].val = v; e->slots[i].mut = mut; return;
        }
    }
    if (e->len >= e->cap) {
        e->cap *= 2; e->slots = realloc(e->slots, e->cap * sizeof(EnvSlot));
    }
    strncpy(e->slots[e->len].name, name, MAX_IDENT - 1);
    e->slots[e->len].val = v; e->slots[e->len].mut = mut;
    e->len++;
}

Value *env_get(Env *e, const char *name) {
    for (Env *cur = e; cur; cur = cur->parent)
        for (int i = 0; i < cur->len; i++)
            if (!strcmp(cur->slots[i].name, name)) return cur->slots[i].val;
    return NULL;
}

int env_set(Env *e, const char *name, Value *v, Interp *I) {
    for (Env *cur = e; cur; cur = cur->parent)
        for (int i = 0; i < cur->len; i++)
            if (!strcmp(cur->slots[i].name, name)) {
                if (!cur->slots[i].mut) { vel_err(I, 0, "Cannot reassign immutable '%s'", name); return 0; }
                cur->slots[i].val = v; return 1;
            }
    vel_err(I, 0, "Undefined variable '%s'", name); return 0;
}
