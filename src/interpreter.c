/* interpreter.c — VEL tree-walking interpreter. Pure C99, zero deps. */
#include "vel.h"

static Value *eval(Interp *I, Node *n, Env *env);
static void   exec(Interp *I, Node *n, Env *env);
static void   exec_list(Interp *I, NodeList *stmts, Env *env);
static int    match_pat(Interp *I, Node *pat, Value *val, Env *env);

void interp_init(Interp *I) { memset(I, 0, sizeof(Interp)); }

Value *interp_run(Interp *I, Node *prog, Env *env) {
    if (!prog || !prog->kids) return v_nil();
    Value *last = v_nil();
    for (int i = 0; i < prog->kids->len; i++) {
        if (I->had_err) break;
        Node *stmt = prog->kids->data[i];
        exec(I, stmt, env);
        if (I->sig == SIG_RET && !I->had_err) {
            last = I->retval ? I->retval : v_nil();
            I->sig = SIG_NONE; I->retval = NULL;
        }
    }
    return last;
}

Value *interp_eval(Interp *I, Node *n, Env *env) { return eval(I, n, env); }
void   interp_exec(Interp *I, Node *n, Env *env) { exec(I, n, env); }

/* ── Arithmetic helpers ── */
static double tonum(Value *v) { return v->kind==V_INT?(double)v->ival:v->fval; }
static int both_int(Value *a, Value *b) { return a->kind==V_INT&&b->kind==V_INT; }

static Value *add_vals(Interp *I, Value *a, Value *b, int line) {
    if (a->kind==V_TEXT||b->kind==V_TEXT) {
        char sa[MAX_STR],sb[MAX_STR],res[MAX_STR*2];
        v_tostr(a,sa,sizeof(sa)); v_tostr(b,sb,sizeof(sb));
        snprintf(res,sizeof(res),"%s%s",sa,sb); return v_text(res);
    }
    if (a->kind==V_LIST&&b->kind==V_LIST) {
        Value *r=v_list();
        for(int i=0;i<a->list.len;i++) v_list_push(r,a->list.items[i]);
        for(int i=0;i<b->list.len;i++) v_list_push(r,b->list.items[i]);
        return r;
    }
    if (both_int(a,b)) return v_int(a->ival+b->ival);
    if ((a->kind==V_INT||a->kind==V_FLOAT)&&(b->kind==V_INT||b->kind==V_FLOAT))
        return v_float(tonum(a)+tonum(b));
    vel_err(I,line,"Cannot add %s and %s",v_typename(a),v_typename(b)); return v_nil();
}

/* ── Function call ── */
Value *call_fn(Interp *I, VelFn *fn, Value **args, int argc) {
    if (I->depth >= MAX_CALL_DEPTH) { vel_err(I,0,"Stack overflow"); return v_nil(); }
    I->depth++;
    Env *child = env_new(fn->closure);
    int n = fn->paramc < argc ? fn->paramc : argc;
    for (int i = 0; i < n; i++) env_def(child, fn->params[i], args[i], 0);

    int saved_sig = I->sig; Value *saved_ret = I->retval;
    I->sig = SIG_NONE; I->retval = NULL;
    exec_list(I, fn->body, child);
    Value *result = I->retval ? I->retval : v_nil();
    I->sig = saved_sig; I->retval = saved_ret;
    I->depth--;
    return result;
}

/* ── eval ── */
static Value *eval(Interp *I, Node *n, Env *env) {
    if (!n || I->had_err) return v_nil();
    switch (n->kind) {
    case N_INT:   return v_int(n->ival);
    case N_FLOAT: return v_float(n->fval);
    case N_STR:   return v_text(n->sval ? n->sval : "");
    case N_BOOL:  return v_bool(n->bval);
    case N_NIL:   return v_none();

    case N_IDENT: {
        Value *v = env_get(env, n->name);
        if (!v) { vel_err(I, n->line, "Undefined variable '%s'", n->name); return v_nil(); }
        return v;
    }

    case N_LIST: {
        Value *r = v_list();
        if (n->kids) for (int i = 0; i < n->kids->len; i++) {
            Value *item = eval(I, n->kids->data[i], env);
            if (I->had_err) return v_nil();
            v_list_push(r, item);
        }
        return r;
    }

    case N_MAP: {
        Value *r = v_map();
        if (n->kids) for (int i = 0; i < n->kids->len && i < MAX_LIST; i++) {
            Value *val = eval(I, n->kids->data[i], env);
            if (I->had_err) return v_nil();
            /* key comes from names2 (string), or fall back to index */
            const char *kname = (n->names2 && i < n->names2->len) ? n->names2->data[i] : "";
            r->map.keys[r->map.len] = v_text(kname);
            r->map.vals[r->map.len] = val;
            r->map.len++;
        }
        return r;
    }

    case N_SOME: { Value *x=eval(I,n->a,env); return I->had_err?v_nil():v_some(x); }
    case N_OK:   { Value *x=eval(I,n->a,env); return I->had_err?v_nil():v_ok(x); }
    case N_FAIL: { Value *x=eval(I,n->a,env); if(I->had_err)return v_nil(); char buf[MAX_STR]; return v_fail(v_tostr(x,buf,sizeof(buf))); }

    case N_BINOP: {
        Value *l=eval(I,n->a,env); if(I->had_err)return v_nil();
        Value *r=eval(I,n->b,env); if(I->had_err)return v_nil();
        switch(n->op){
            case OP_ADD: return add_vals(I,l,r,n->line);
            case OP_SUB: if(both_int(l,r))return v_int(l->ival-r->ival); return v_float(tonum(l)-tonum(r));
            case OP_MUL:
                if(both_int(l,r))return v_int(l->ival*r->ival);
                if(l->kind==V_TEXT&&r->kind==V_INT){ char buf[MAX_STR]; buf[0]=0; for(i64 k=0;k<r->ival;k++) strncat(buf,l->sval,MAX_STR-strlen(buf)-1); return v_text(buf); }
                return v_float(tonum(l)*tonum(r));
            case OP_DIV: if(tonum(r)==0.0){vel_err(I,n->line,"Division by zero");return v_nil();} if(both_int(l,r))return v_int(l->ival/r->ival); return v_float(tonum(l)/tonum(r));
            case OP_MOD: if(!both_int(l,r)){vel_err(I,n->line,"Modulo needs Int");return v_nil();} return v_int(l->ival%r->ival);
            case OP_EQ:   return v_bool(v_equal(l,r));
            case OP_NEQ:  return v_bool(!v_equal(l,r));
            case OP_LT:   return v_bool(both_int(l,r)?l->ival<r->ival :tonum(l)<tonum(r));
            case OP_LTEQ: return v_bool(both_int(l,r)?l->ival<=r->ival:tonum(l)<=tonum(r));
            case OP_GT:   return v_bool(both_int(l,r)?l->ival>r->ival :tonum(l)>tonum(r));
            case OP_GTEQ: return v_bool(both_int(l,r)?l->ival>=r->ival:tonum(l)>=tonum(r));
            case OP_AND:  return v_bool(v_truthy(l)&&v_truthy(r));
            case OP_OR:   return v_bool(v_truthy(l)||v_truthy(r));
            default: vel_err(I,n->line,"Unknown op"); return v_nil();
        }
    }

    case N_UNARY: {
        Value *v=eval(I,n->a,env); if(I->had_err)return v_nil();
        if(n->op==OP_NEG){ if(v->kind==V_INT)return v_int(-v->ival); if(v->kind==V_FLOAT)return v_float(-v->fval); vel_err(I,n->line,"Cannot negate %s",v_typename(v));return v_nil(); }
        if(n->op==OP_NOT) return v_bool(!v_truthy(v));
        return v_nil();
    }

    case N_CALL: {
        Value *callee=eval(I,n->a,env); if(I->had_err)return v_nil();
        Value *args[MAX_PARAMS]; int argc=0;
        if(n->kids) for(int i=0;i<n->kids->len&&i<MAX_PARAMS;i++){
            args[i]=eval(I,n->kids->data[i],env); if(I->had_err)return v_nil(); argc++;
        }
        if(callee->kind==V_NATIVE) return stdlib_call(callee->nname,args,argc,I);
        if(callee->kind==V_FN)     return call_fn(I,&callee->fn,args,argc);
        if(callee->kind==V_CLASS) {
            /* Constructor */
            Value *inst=(Value*)calloc(1,sizeof(Value)); inst->kind=V_INST;
            snprintf(inst->inst.class_name, MAX_IDENT, "%s", callee->cls.name);
            inst->inst.fieldc  = callee->cls.fieldc;
            inst->inst.fnames  = callee->cls.fields;
            inst->inst.fvals   = calloc(callee->cls.fieldc,sizeof(Value*));
            for(int i=0;i<callee->cls.fieldc;i++) inst->inst.fvals[i]=i<argc?args[i]:v_nil();
            inst->inst.methods = callee->cls.methods;
            inst->inst.methodc = callee->cls.methodc;
            return inst;
        }
        vel_err(I,n->line,"'%s' is not callable",v_typename(callee)); return v_nil();
    }

    case N_MCALL: {
        Value *obj=eval(I,n->a,env); if(I->had_err)return v_nil();
        Value *args[MAX_PARAMS]; int argc=0;
        if(n->kids) for(int i=0;i<n->kids->len&&i<MAX_PARAMS;i++){
            args[i]=eval(I,n->kids->data[i],env); if(I->had_err)return v_nil(); argc++;
        }
        /* Instance method lookup */
        if(obj->kind==V_INST){
            for(int i=0;i<obj->inst.methodc;i++){
                if(!strcmp(obj->inst.methods[i].name,n->name)){
                    VelFn *fn = &obj->inst.methods[i];
                    /* Create a child env with all instance fields pre-defined
                       so method body can reference `name` directly (implicit self) */
                    Env *method_env = env_new(fn->closure);
                    for(int f=0;f<obj->inst.fieldc;f++)
                        env_def(method_env, obj->inst.fnames[f],
                                obj->inst.fvals[f], 0);
                    /* also bind explicit params */
                    int np = fn->paramc < argc ? fn->paramc : argc;
                    for(int j=0;j<np;j++) env_def(method_env, fn->params[j], args[j], 0);
                    /* run the method body */
                    if (I->depth >= MAX_CALL_DEPTH) { vel_err(I,n->line,"Stack overflow"); return v_nil(); }
                    I->depth++;
                    int saved_sig = I->sig; Value *saved_ret = I->retval;
                    I->sig = SIG_NONE; I->retval = NULL;
                    exec_list(I, fn->body, method_env);
                    Value *result = I->retval ? I->retval : v_nil();
                    if(I->sig==SIG_RET && !I->had_err) { I->sig=saved_sig; I->retval=saved_ret; }
                    I->depth--;
                    return result;
                }
            }
        }
        return stdlib_method(obj,n->name,args,argc,I);
    }

    case N_FIELD: {
        Value *obj=eval(I,n->a,env); if(I->had_err)return v_nil();
        if(obj->kind==V_INST){
            for(int i=0;i<obj->inst.fieldc;i++)
                if(!strcmp(obj->inst.fnames[i],n->name)) return obj->inst.fvals[i];
        }
        vel_err(I,n->line,"No field '%s' on %s",n->name,v_typename(obj)); return v_nil();
    }

    case N_INDEX: {
        Value *obj=eval(I,n->a,env); if(I->had_err)return v_nil();
        Value *idx=eval(I,n->b,env); if(I->had_err)return v_nil();
        if(obj->kind==V_LIST&&idx->kind==V_INT){
            i64 i=idx->ival; if(i<0)i+=obj->list.len;
            if(i<0||i>=(i64)obj->list.len){vel_err(I,n->line,"List index out of bounds");return v_nil();}
            return obj->list.items[(int)i];
        }
        if(obj->kind==V_MAP){
            for(int i=0;i<obj->map.len;i++) if(v_equal(obj->map.keys[i],idx)) return obj->map.vals[i];
            return v_nil();
        }
        if(obj->kind==V_TEXT&&idx->kind==V_INT){
            int i=(int)idx->ival, len=(int)strlen(obj->sval);
            if(i<0||i>=len){vel_err(I,n->line,"String index out of bounds");return v_nil();}
            char buf[2]={obj->sval[i],0}; return v_text(buf);
        }
        vel_err(I,n->line,"Cannot index %s",v_typename(obj)); return v_nil();
    }

    case N_LAMBDA: {
        Value *v=(Value*)calloc(1,sizeof(Value)); v->kind=V_FN;
        strcpy(v->fn.name,"<lambda>");
        v->fn.paramc = n->params ? n->params->len : 0;
        v->fn.params = calloc(v->fn.paramc, sizeof(char*));
        for(int i=0;i<v->fn.paramc;i++) v->fn.params[i]=strdup(n->params->data[i]);
        /* wrap body expr in a return node */
        Node *ret=node_alloc(N_RET,n->line); ret->a=n->a;
        v->fn.body=nl_new(); nl_push(v->fn.body,ret);
        v->fn.closure=env;
        return v;
    }

    case N_WAIT:  return eval(I,n->a,env); /* sync stub */

    case N_PROP: {
        Value *v=eval(I,n->a,env); if(I->had_err)return v_nil();
        if(v->kind==V_RESULT){
            if(v->res.is_ok) return v->res.inner;
            I->sig=SIG_RET; I->retval=v_fail(v->res.err); return v_nil();
        }
        return v;
    }

    case N_IN: {
        Value *val=eval(I,n->a,env); if(I->had_err)return v_nil();
        Value *col=eval(I,n->b,env); if(I->had_err)return v_nil();
        if(col->kind==V_LIST){ for(int i=0;i<col->list.len;i++) if(v_equal(col->list.items[i],val))return v_bool(1); return v_bool(0); }
        if(col->kind==V_TEXT&&val->kind==V_TEXT) return v_bool(strstr(col->sval,val->sval)!=NULL);
        return v_bool(0);
    }

    case N_IS: {
        Value *v=eval(I,n->a,env); if(I->had_err)return v_nil();
        if(!strcmp(n->name,"ok"))   return v_bool(v->kind==V_RESULT&&v->res.is_ok);
        if(!strcmp(n->name,"fail")) return v_bool(v->kind==V_RESULT&&!v->res.is_ok);
        if(!strcmp(n->name,"some")) return v_bool(v->kind==V_OPTION&&v->opt.is_some);
        if(!strcmp(n->name,"none")) return v_bool(v->kind==V_OPTION&&!v->opt.is_some);
        return v_bool(!strcmp(v_typename(v),n->name));
    }

    case N_ASK: {
        Value *q=eval(I,n->b,env);
        char buf[MAX_STR/2], res[MAX_STR];
        v_tostr(q,buf,sizeof(buf));
        snprintf(res,sizeof(res),"[AI result for: %.2000s]",buf);
        return v_ok(v_text(res));
    }
    case N_INFER: {
        Value *q=eval(I,n->b,env);
        char buf[MAX_STR/2], res[MAX_STR];
        v_tostr(q,buf,sizeof(buf));
        snprintf(res,sizeof(res),"[AI inference: %.2000s]",buf);
        return v_text(res);
    }

    /* Inline if expression — evaluate condition, run the appropriate branch, capture SIG_RET */
    case N_IF: {
        Value *cond = eval(I,n->a,env); if(I->had_err) return v_nil();
        Env *child = env_new(env);
        int saved_sig = I->sig; Value *saved_ret = I->retval;
        I->sig = SIG_NONE; I->retval = NULL;
        if(v_truthy(cond)) exec_list(I,n->kids,child);
        else if(n->kids2)  exec_list(I,n->kids2,child);
        Value *result = I->retval ? I->retval : v_nil();
        if(I->sig == SIG_RET && !I->had_err) { I->sig = saved_sig; I->retval = saved_ret; }
        return result;
    }

    case N_RANGE: {
        Value *s=eval(I,n->a,env),*e=eval(I,n->b,env); if(I->had_err)return v_nil();
        if(s->kind!=V_INT||e->kind!=V_INT){vel_err(I,n->line,"Range needs Int");return v_nil();}
        Value *r=v_list(); for(i64 i=s->ival;i<e->ival;i++) v_list_push(r,v_int(i)); return r;
    }

    default: exec(I,n,env); return v_nil();
    }
}

/* ── exec ── */
static void exec(Interp *I, Node *n, Env *env) {
    if (!n || I->had_err) return;
    switch (n->kind) {

    case N_LET: {
        Value *v=eval(I,n->a,env); if(I->had_err)return;
        env_def(env,n->name,v,n->mutable);
        break;
    }

    case N_ASSIGN: {
        Value *v=eval(I,n->b,env); if(I->had_err)return;
        const char *nm=(n->a->kind==N_IDENT)?n->a->name:NULL;
        if(!nm){vel_err(I,n->line,"Complex assignment not supported");return;}
        if(n->bval==1){ /* += */
            Value *cur=env_get(env,nm); if(!cur){vel_err(I,n->line,"Undefined '%s'",nm);return;}
            v=add_vals(I,cur,v,n->line);
        } else if(n->bval==2){ /* -= */
            Value *cur=env_get(env,nm); if(!cur){vel_err(I,n->line,"Undefined '%s'",nm);return;}
            v=both_int(cur,v)?v_int(cur->ival-v->ival):v_float(tonum(cur)-tonum(v));
        }
        env_set(env,nm,v,I);
        break;
    }

    case N_FN: {
        Value *fv=(Value*)calloc(1,sizeof(Value)); fv->kind=V_FN;
        snprintf(fv->fn.name, MAX_IDENT, "%s",n->name);
        fv->fn.paramc = n->params ? n->params->len : 0;
        fv->fn.params = calloc(fv->fn.paramc,sizeof(char*));
        for(int i=0;i<fv->fn.paramc;i++) fv->fn.params[i]=strdup(n->params->data[i]);
        fv->fn.body    = n->kids;
        fv->fn.closure = env;
        env_def(env,n->name,fv,0);
        break;
    }

    case N_CLASS: {
        Value *cv=(Value*)calloc(1,sizeof(Value)); cv->kind=V_CLASS;
        snprintf(cv->cls.name, MAX_IDENT, "%s",n->name);

        /* If class extends a parent, inherit its fields and methods first */
        int parent_fieldc = 0;
        char **parent_fields = NULL;
        VelFn *parent_methods = NULL;
        int parent_methodc = 0;
        if (n->sval) { /* superclass name stored in sval */
            Value *parent = env_get(env, n->sval);
            if (parent && parent->kind == V_CLASS) {
                parent_fieldc  = parent->cls.fieldc;
                parent_fields  = parent->cls.fields;
                parent_methods = parent->cls.methods;
                parent_methodc = parent->cls.methodc;
            }
        }

        /* Combine parent fields + own fields */
        int own_fieldc = n->names2 ? n->names2->len : 0;
        cv->cls.fieldc = parent_fieldc + own_fieldc;
        cv->cls.fields = calloc(cv->cls.fieldc, sizeof(char*));
        for(int i=0;i<parent_fieldc;i++) cv->cls.fields[i]=strdup(parent_fields[i]);
        for(int i=0;i<own_fieldc;i++) cv->cls.fields[parent_fieldc+i]=strdup(n->names2->data[i]);

        /* Combine parent methods + own methods (own overrides parent) */
        int own_methodc = n->kids ? n->kids->len : 0;
        /* start with parent methods, then add/override with own */
        int total_methods = parent_methodc + own_methodc;
        cv->cls.methods = calloc(total_methods, sizeof(VelFn));
        /* copy parent methods */
        for(int i=0;i<parent_methodc;i++) cv->cls.methods[i]=parent_methods[i];
        cv->cls.methodc = parent_methodc;
        /* add/override with own methods */
        for(int i=0;i<own_methodc;i++){
            Node *m=n->kids->data[i]; if(m->kind!=N_FN) continue;
            /* check if this overrides a parent method */
            int found = -1;
            for(int j=0;j<cv->cls.methodc;j++){
                if(!strcmp(cv->cls.methods[j].name, m->name)){ found=j; break; }
            }
            int slot = (found >= 0) ? found : cv->cls.methodc++;
            VelFn *fn=&cv->cls.methods[slot];
            snprintf(fn->name, MAX_IDENT, "%s",m->name);
            fn->paramc = m->params ? m->params->len : 0;
            fn->params = calloc(fn->paramc,sizeof(char*));
            for(int j=0;j<fn->paramc;j++) fn->params[j]=strdup(m->params->data[j]);
            fn->body    = m->kids;
            fn->closure = env;
        }
        env_def(env,n->name,cv,0);
        break;
    }

    case N_IF: {
        Value *cond=eval(I,n->a,env); if(I->had_err)return;
        Env *child=env_new(env);
        if(v_truthy(cond)) exec_list(I,n->kids,child);
        else if(n->kids2)  exec_list(I,n->kids2,child);
        break;
    }

    case N_MATCH: {
        Value *subj=eval(I,n->a,env); if(I->had_err)return;
        if(!n->kids) break;
        for(int i=0;i<n->kids->len;i++){
            Node *arm=n->kids->data[i]; if(arm->kind!=N_ARM) continue;
            Env *child=env_new(env);
            if(match_pat(I,arm->a,subj,child)){
                /* if arm body is a statement (say/log/run), exec it;
                   otherwise eval it and return the value */
                NK bk = arm->b ? arm->b->kind : N_NIL;
                if (bk==N_SAY||bk==N_LOG||bk==N_RUN||bk==N_EXPR) {
                    exec(I,arm->b,child);
                } else {
                    Value *v=eval(I,arm->b,child);
                    if(!I->had_err){ I->sig=SIG_RET; I->retval=v; }
                }
                break;
            }
        }
        break;
    }

    case N_FOR: {
        Value *iter=eval(I,n->a,env); if(I->had_err)return;
        Value **items=NULL; int cnt=0; Value *tmp=NULL;
        if(iter->kind==V_LIST){items=iter->list.items;cnt=iter->list.len;}
        else if(iter->kind==V_TEXT){
            tmp=v_list(); int len=(int)strlen(iter->sval);
            for(int i=0;i<len;i++){char buf[2]={iter->sval[i],0};v_list_push(tmp,v_text(buf));}
            items=tmp->list.items; cnt=tmp->list.len;
        } else {vel_err(I,n->line,"Cannot iterate %s",v_typename(iter));return;}
        for(int i=0;i<cnt&&!I->had_err;i++){
            Env *child=env_new(env); env_def(child,n->name,items[i],0);
            exec_list(I,n->kids,child);
            if(I->sig==SIG_BRK){I->sig=SIG_NONE;break;}
            if(I->sig==SIG_RET) break;
        }
        break;
    }

    case N_WHILE: {
        while(!I->had_err){
            Value *c=eval(I,n->a,env); if(!v_truthy(c)) break;
            Env *child=env_new(env); exec_list(I,n->kids,child);
            if(I->sig==SIG_BRK){I->sig=SIG_NONE;break;}
            if(I->sig==SIG_RET) break;
        }
        break;
    }

    case N_REPEAT: {
        Value *cnt=eval(I,n->a,env); if(I->had_err)return;
        if(cnt->kind!=V_INT){vel_err(I,n->line,"repeat needs Int");return;}
        for(i64 k=0;k<cnt->ival&&!I->had_err;k++){
            Env *child=env_new(env); exec_list(I,n->kids,child);
            if(I->sig==SIG_BRK){I->sig=SIG_NONE;break;}
            if(I->sig==SIG_RET) break;
        }
        break;
    }

    case N_RET: {
        Value *v=n->a?eval(I,n->a,env):v_nil();
        I->sig=SIG_RET; I->retval=v; break;
    }

    case N_SAY: { Value *v=eval(I,n->a,env); if(!I->had_err){v_print(v,stdout);printf("\n");} break; }
    case N_LOG: { Value *v=eval(I,n->a,env); if(!I->had_err){v_print(v,stderr);fprintf(stderr,"\n");} break; }
    case N_RUN: eval(I,n->a,env); break;
    case N_USE: break; /* module loading handled at stdlib registration */

    case N_ROUTE: {
        Value *subj=eval(I,n->a,env); if(I->had_err)return;
        char sbuf[MAX_STR]; v_tostr(subj,sbuf,sizeof(sbuf));
        for(int i=0;i<(int)strlen(sbuf);i++) sbuf[i]=(char)tolower((u8)sbuf[i]);
        int matched=0;
        if(n->kids) for(int i=0;i<n->kids->len&&!matched;i++){
            Node *arm=n->kids->data[i]; if(arm->kind!=N_RARM) continue;
            Value *intent=eval(I,arm->a,env); if(I->had_err)return;
            char ibuf[MAX_STR]; v_tostr(intent,ibuf,sizeof(ibuf));
            for(int j=0;ibuf[j];j++) ibuf[j]=(char)tolower((u8)ibuf[j]);
            if(strstr(sbuf,ibuf)){
                /* arm->b is the handler (may be N_SAY, N_LOG, or expression) */
                if(arm->b){
                    NK bk=arm->b->kind;
                    if(bk==N_SAY||bk==N_LOG||bk==N_RUN) exec(I,arm->b,env);
                    else eval(I,arm->b,env);
                }
                matched=1;
            }
        }
        if(!matched&&n->b){
            NK bk=n->b->kind;
            if(bk==N_SAY||bk==N_LOG||bk==N_RUN) exec(I,n->b,env);
            else eval(I,n->b,env);
        }
        break;
    }

    case N_RARM: { Value *v=eval(I,n->b,env); (void)v; break; } /* call the handler expr */

    case N_EXPR: eval(I,n->a,env); break;

    default: eval(I,n,env); break;
    }
}

static void exec_list(Interp *I, NodeList *stmts, Env *env) {
    if (!stmts) return;
    for(int i=0;i<stmts->len&&!I->had_err&&I->sig==SIG_NONE;i++)
        exec(I,stmts->data[i],env);
}

static int match_pat(Interp *I, Node *pat, Value *val, Env *env) {
    if (!pat) return 0;
    switch(pat->kind){
        case N_IDENT: if(!strcmp(pat->name,"_"))return 1; env_def(env,pat->name,val,0); return 1;
        case N_NIL:   return val->kind==V_OPTION&&!val->opt.is_some;
        case N_INT:   return val->kind==V_INT&&val->ival==pat->ival;
        case N_FLOAT: return val->kind==V_FLOAT&&val->fval==pat->fval;
        case N_STR:   return val->kind==V_TEXT&&!strcmp(val->sval,pat->sval?pat->sval:"");
        case N_BOOL:  return val->kind==V_BOOL&&val->bval==pat->bval;
        case N_OK:
            if(val->kind!=V_RESULT||!val->res.is_ok) return 0;
            if(pat->name[0]) env_def(env,pat->name,val->res.inner,0);
            return 1;
        case N_FAIL:
            if(val->kind!=V_RESULT||val->res.is_ok) return 0;
            if(pat->name[0]) env_def(env,pat->name,v_text(val->res.err?val->res.err:""),0);
            return 1;
        case N_SOME:
            if(val->kind!=V_OPTION||!val->opt.is_some) return 0;
            if(pat->name[0]) env_def(env,pat->name,val->opt.inner,0);
            return 1;
        default: {
            /* Expression pattern — evaluate and check truthiness (or compare to subject) */
            Value *pval = eval(I, pat, env);
            if (I->had_err) return 0;
            /* If subject is `true` (match true:), pattern is a boolean expr */
            if (val->kind == V_BOOL && val->bval == 1) return v_truthy(pval);
            return v_equal(pval, val);
        }
    }
}
