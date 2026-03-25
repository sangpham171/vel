/* value.c — Runtime values for VEL */
#include "vel.h"

static Value *valloc(VK k) {
    Value *v = calloc(1, sizeof(Value));
    v->kind = k;
    return v;
}

Value *v_nil(void)        { return valloc(V_NIL); }
Value *v_bool(int b)      { Value *v = valloc(V_BOOL);  v->bval = b ? 1 : 0; return v; }
Value *v_int(i64 n)       { Value *v = valloc(V_INT);   v->ival = n; return v; }
Value *v_float(double f)  { Value *v = valloc(V_FLOAT); v->fval = f; return v; }

Value *v_text(const char *s) {
    Value *v = valloc(V_TEXT);
    v->sval = strdup(s ? s : "");
    return v;
}

Value *v_list(void) {
    Value *v = valloc(V_LIST);
    v->list.cap   = 8;
    v->list.len   = 0;
    v->list.items = calloc(8, sizeof(Value*));
    return v;
}

void v_list_push(Value *v, Value *item) {
    if (v->list.len >= v->list.cap) {
        v->list.cap *= 2;
        v->list.items = realloc(v->list.items, v->list.cap * sizeof(Value*));
    }
    v->list.items[v->list.len++] = item;
}

Value *v_map(void) {
    Value *v = valloc(V_MAP);
    v->map.keys = calloc(MAX_LIST, sizeof(Value*));
    v->map.vals = calloc(MAX_LIST, sizeof(Value*));
    v->map.len  = 0;
    return v;
}

Value *v_none(void)           { Value *v = valloc(V_OPTION); v->opt.is_some = 0; return v; }
Value *v_some(Value *x)       { Value *v = valloc(V_OPTION); v->opt.is_some = 1; v->opt.inner = x; return v; }
Value *v_ok(Value *x)         { Value *v = valloc(V_RESULT); v->res.is_ok = 1; v->res.inner = x; return v; }
Value *v_fail(const char *m)  { Value *v = valloc(V_RESULT); v->res.is_ok = 0; v->res.err = strdup(m ? m : "error"); return v; }
Value *v_native(const char *n){ Value *v = valloc(V_NATIVE); snprintf(v->nname, MAX_IDENT, "%s", n); return v; }

int v_truthy(Value *v) {
    if (!v) return 0;
    switch (v->kind) {
        case V_NIL:    return 0;
        case V_BOOL:   return v->bval;
        case V_INT:    return v->ival != 0;
        case V_FLOAT:  return v->fval != 0.0;
        case V_TEXT:   return v->sval && v->sval[0];
        case V_LIST:   return v->list.len > 0;
        case V_OPTION: return v->opt.is_some;
        case V_RESULT: return v->res.is_ok;
        default:       return 1;
    }
}

int v_equal(Value *a, Value *b) {
    if (!a || !b) return a == b;
    if (a->kind == V_INT && b->kind == V_FLOAT) return (double)a->ival == b->fval;
    if (a->kind == V_FLOAT && b->kind == V_INT) return a->fval == (double)b->ival;
    if (a->kind != b->kind) return 0;
    switch (a->kind) {
        case V_NIL:   return 1;
        case V_BOOL:  return a->bval == b->bval;
        case V_INT:   return a->ival == b->ival;
        case V_FLOAT: return a->fval == b->fval;
        case V_TEXT:  return strcmp(a->sval, b->sval) == 0;
        case V_OPTION:
            if (a->opt.is_some != b->opt.is_some) return 0;
            return !a->opt.is_some || v_equal(a->opt.inner, b->opt.inner);
        default: return 0;
    }
}

const char *v_typename(Value *v) {
    if (!v) return "nil";
    switch (v->kind) {
        case V_NIL:    return "Nil";
        case V_BOOL:   return "Bool";
        case V_INT:    return "Int";
        case V_FLOAT:  return "Float";
        case V_TEXT:   return "Text";
        case V_LIST:   return "List";
        case V_MAP:    return "Map";
        case V_OPTION: return "Option";
        case V_RESULT: return "Result";
        case V_FN:     return "Function";
        case V_CLASS:  return "Class";
        case V_INST:   return "Instance";
        case V_NATIVE: return "NativeFunction";
        default:       return "Unknown";
    }
}

char *v_tostr(Value *v, char *buf, int sz) {
    if (!v) { strncpy(buf, "nil", sz); return buf; }
    switch (v->kind) {
        case V_NIL:   strncpy(buf, "nil", sz); break;
        case V_BOOL:  strncpy(buf, v->bval ? "true" : "false", sz); break;
        case V_INT:   snprintf(buf, sz, "%lld", (long long)v->ival); break;
        case V_FLOAT: {
            snprintf(buf, sz, "%g", v->fval);
            if (!strchr(buf, '.') && !strchr(buf, 'e'))
                strncat(buf, ".0", sz - strlen(buf) - 1);
            break;
        }
        case V_TEXT:  strncpy(buf, v->sval ? v->sval : "", sz); break;
        case V_LIST: {
            int off = snprintf(buf, sz, "[");
            for (int i = 0; i < v->list.len && off < sz-4; i++) {
                char tmp[256]; v_tostr(v->list.items[i], tmp, sizeof(tmp));
                off += snprintf(buf+off, sz-off, "%s%s", tmp, i<v->list.len-1?", ":"");
            }
            snprintf(buf+off, sz-off, "]");
            break;
        }
        case V_MAP: {
            int off = snprintf(buf, sz, "{");
            for (int i = 0; i < v->map.len && off < sz-8; i++) {
                char k[128], vl[256];
                v_tostr(v->map.keys[i], k, sizeof(k));
                v_tostr(v->map.vals[i], vl, sizeof(vl));
                off += snprintf(buf+off, sz-off, "%s: %s%s", k, vl, i<v->map.len-1?", ":"");
            }
            snprintf(buf+off, sz-off, "}");
            break;
        }
        case V_OPTION:
            if (!v->opt.is_some) strncpy(buf, "none", sz);
            else { char t[256]; v_tostr(v->opt.inner, t, sizeof(t)); snprintf(buf, sz, "some(%s)", t); }
            break;
        case V_RESULT:
            if (v->res.is_ok) { char t[256]; v_tostr(v->res.inner, t, sizeof(t)); snprintf(buf, sz, "ok(%s)", t); }
            else snprintf(buf, sz, "fail(\"%s\")", v->res.err ? v->res.err : "");
            break;
        case V_FN:     snprintf(buf, sz, "<fn %s>", v->fn.name); break;
        case V_CLASS:  snprintf(buf, sz, "<class %s>", v->cls.name); break;
        case V_INST:   snprintf(buf, sz, "<%s instance>", v->inst.class_name); break;
        case V_NATIVE: snprintf(buf, sz, "<native %s>", v->nname); break;
        default:       strncpy(buf, "<value>", sz);
    }
    return buf;
}

void v_print(Value *v, FILE *fp) {
    char buf[MAX_STR]; v_tostr(v, buf, sizeof(buf)); fprintf(fp, "%s", buf);
}
