/* stdlib.c — Vel built-in functions and methods. Pure C99, zero deps. */
#include "vel.h"
#include <math.h>

static const char *NATIVES[] = {
    "say","log","print","len","count","toText","toInt","toFloat","toBool",
    "abs","min","max","floor","ceil","round","sqrt","pow",
    "range","first","last","reverse","keys","values","type_of","now",
    "sum","join","split","contains",
    NULL
};

void stdlib_register(Env *env) {
    for (int i = 0; NATIVES[i]; i++)
        env_def(env, NATIVES[i], v_native(NATIVES[i]), 0);
}

static double to_num(Value *v) {
    if (v->kind==V_INT) return (double)v->ival;
    if (v->kind==V_FLOAT) return v->fval;
    return 0.0;
}

Value *stdlib_call(const char *name, Value **a, int argc, Interp *I) {
    /* I/O */
    if (!strcmp(name,"say")||!strcmp(name,"print")) {
        if (argc>0) { v_print(a[0], stdout); }
        printf("\n"); return v_nil();
    }
    if (!strcmp(name,"log")) {
        if (argc>0) { v_print(a[0], stderr); }
        fprintf(stderr,"\n"); return v_nil();
    }
    /* Type conversion */
    if (!strcmp(name,"toText")) {
        if (!argc) return v_text("");
        char buf[MAX_STR]; return v_text(v_tostr(a[0],buf,sizeof(buf)));
    }
    if (!strcmp(name,"toInt")) {
        if (!argc) return v_int(0);
        switch(a[0]->kind){
            case V_INT:   return v_int(a[0]->ival);
            case V_FLOAT: return v_int((i64)a[0]->fval);
            case V_BOOL:  return v_int(a[0]->bval);
            case V_TEXT: { char *e; i64 n=(i64)strtoll(a[0]->sval,&e,10);
                           if(*e){vel_err(I,0,"Cannot convert '%s' to Int",a[0]->sval);return v_nil();}
                           return v_int(n); }
            default: vel_err(I,0,"Cannot convert to Int"); return v_nil();
        }
    }
    if (!strcmp(name,"toFloat")) {
        if (!argc) return v_float(0);
        switch(a[0]->kind){
            case V_FLOAT: return v_float(a[0]->fval);
            case V_INT:   return v_float((double)a[0]->ival);
            case V_TEXT:  return v_float(atof(a[0]->sval));
            default: vel_err(I,0,"Cannot convert to Float"); return v_nil();
        }
    }
    if (!strcmp(name,"toBool")) { return argc ? v_bool(v_truthy(a[0])) : v_bool(0); }
    /* Math */
    if (!strcmp(name,"abs")) {
        if (!argc){vel_err(I,0,"abs() needs 1 arg");return v_nil();}
        if (a[0]->kind==V_INT)   return v_int(a[0]->ival<0?-a[0]->ival:a[0]->ival);
        if (a[0]->kind==V_FLOAT) return v_float(fabs(a[0]->fval));
        vel_err(I,0,"abs() needs a number"); return v_nil();
    }
    if (!strcmp(name,"min")) {
        if (argc<2){vel_err(I,0,"min() needs 2 args");return v_nil();}
        if (a[0]->kind==V_INT&&a[1]->kind==V_INT) return v_int(a[0]->ival<a[1]->ival?a[0]->ival:a[1]->ival);
        double x=to_num(a[0]),y=to_num(a[1]); return v_float(x<y?x:y);
    }
    if (!strcmp(name,"max")) {
        if (argc<2){vel_err(I,0,"max() needs 2 args");return v_nil();}
        if (a[0]->kind==V_INT&&a[1]->kind==V_INT) return v_int(a[0]->ival>a[1]->ival?a[0]->ival:a[1]->ival);
        double x=to_num(a[0]),y=to_num(a[1]); return v_float(x>y?x:y);
    }
    if (!strcmp(name,"floor")) { if (!argc) return v_int(0); if (a[0]->kind==V_INT) return v_int(a[0]->ival); return v_int((i64)floor(to_num(a[0]))); }
    if (!strcmp(name,"ceil"))  { if (!argc) return v_int(0); if (a[0]->kind==V_INT) return v_int(a[0]->ival); return v_int((i64)ceil(to_num(a[0]))); }
    if (!strcmp(name,"round")) { if (!argc) return v_int(0); if (a[0]->kind==V_INT) return v_int(a[0]->ival); return v_int((i64)round(to_num(a[0]))); }
    if (!strcmp(name,"sqrt")) { if (!argc) return v_float(0); return v_float(sqrt(to_num(a[0]))); }
    if (!strcmp(name,"pow")) {
        if (argc<2){vel_err(I,0,"pow() needs 2 args");return v_nil();}
        return v_float(pow(to_num(a[0]),to_num(a[1])));
    }
    /* Collections */
    if (!strcmp(name,"len")||!strcmp(name,"count")) {
        if (!argc) return v_int(0);
        switch(a[0]->kind){
            case V_TEXT: return v_int((i64)strlen(a[0]->sval));
            case V_LIST: return v_int((i64)a[0]->list.len);
            case V_MAP:  return v_int((i64)a[0]->map.len);
            default: vel_err(I,0,"len() needs Text, List, or Map"); return v_nil();
        }
    }
    if (!strcmp(name,"first")) { if (!argc||a[0]->kind!=V_LIST) return v_none(); return a[0]->list.len?v_some(a[0]->list.items[0]):v_none(); }
    if (!strcmp(name,"last"))  { if (!argc||a[0]->kind!=V_LIST) return v_none(); int n=a[0]->list.len; return n?v_some(a[0]->list.items[n-1]):v_none(); }
    if (!strcmp(name,"reverse")) {
        if (!argc) return v_nil();
        if (a[0]->kind==V_LIST) { Value *r=v_list(); for(int i=a[0]->list.len-1;i>=0;i--) v_list_push(r,a[0]->list.items[i]); return r; }
        if (a[0]->kind==V_TEXT) { int n=(int)strlen(a[0]->sval); char *buf=malloc(n+1); for(int i=0;i<n;i++) buf[i]=a[0]->sval[n-1-i]; buf[n]=0; Value *r=v_text(buf); free(buf); return r; }
        vel_err(I,0,"reverse() needs List or Text"); return v_nil();
    }
    if (!strcmp(name,"keys")) {
        if (!argc||a[0]->kind!=V_MAP){vel_err(I,0,"keys() needs Map");return v_nil();}
        Value *r=v_list(); for(int i=0;i<a[0]->map.len;i++) v_list_push(r,a[0]->map.keys[i]); return r;
    }
    if (!strcmp(name,"values")) {
        if (!argc||a[0]->kind!=V_MAP){vel_err(I,0,"values() needs Map");return v_nil();}
        Value *r=v_list(); for(int i=0;i<a[0]->map.len;i++) v_list_push(r,a[0]->map.vals[i]); return r;
    }
    if (!strcmp(name,"sum")) {
        if (!argc||a[0]->kind!=V_LIST){vel_err(I,0,"sum() needs List");return v_nil();}
        i64 si=0; double sf=0; int is_f=0;
        for(int i=0;i<a[0]->list.len;i++){
            Value *x=a[0]->list.items[i];
            if(x->kind==V_INT) si+=x->ival;
            else if(x->kind==V_FLOAT){is_f=1;sf+=x->fval;}
            else {vel_err(I,0,"sum() requires numbers");return v_nil();}
        }
        return is_f ? v_float(si+sf) : v_int(si);
    }
    if (!strcmp(name,"contains")) {
        if (argc<2) return v_bool(0);
        if (a[0]->kind==V_LIST) { for(int i=0;i<a[0]->list.len;i++) if(v_equal(a[0]->list.items[i],a[1])) return v_bool(1); return v_bool(0); }
        if (a[0]->kind==V_TEXT&&a[1]->kind==V_TEXT) return v_bool(strstr(a[0]->sval,a[1]->sval)!=NULL);
        return v_bool(0);
    }
    if (!strcmp(name,"range")) {
        if (argc<2){vel_err(I,0,"range() needs 2 args");return v_nil();}
        i64 s=(i64)to_num(a[0]), e=(i64)to_num(a[1]);
        Value *r=v_list(); for(i64 i=s;i<e;i++) v_list_push(r,v_int(i)); return r;
    }
    if (!strcmp(name,"join")) {
        if (!argc||a[0]->kind!=V_LIST){vel_err(I,0,"join() needs List");return v_nil();}
        const char *sep = argc>1&&a[1]->kind==V_TEXT ? a[1]->sval : "";
        char buf[MAX_STR]; int off=0;
        for(int i=0;i<a[0]->list.len&&off<(int)sizeof(buf)-4;i++){
            char tmp[256]; v_tostr(a[0]->list.items[i],tmp,sizeof(tmp));
            off+=snprintf(buf+off,sizeof(buf)-off,"%s%s",tmp,i<a[0]->list.len-1?sep:"");
        }
        return v_text(buf);
    }
    if (!strcmp(name,"split")) {
        if (!argc||a[0]->kind!=V_TEXT){vel_err(I,0,"split() needs Text");return v_nil();}
        const char *sep = argc>1&&a[1]->kind==V_TEXT ? a[1]->sval : " ";
        int slen=(int)strlen(sep); Value *r=v_list();
        const char *cur=a[0]->sval;
        while(*cur){ const char *f=slen?strstr(cur,sep):NULL;
            if(!f){v_list_push(r,v_text(cur));break;}
            char part[MAX_STR]; int plen=(int)(f-cur); strncpy(part,cur,plen); part[plen]=0;
            v_list_push(r,v_text(part)); cur=f+slen; }
        return r;
    }
    if (!strcmp(name,"type_of")) { return argc ? v_text(v_typename(a[0])) : v_text("nil"); }
    if (!strcmp(name,"now"))     { return v_text("2026-01-01T00:00:00Z"); }

    vel_err(I,0,"Unknown function '%s'", name); return v_nil();
}

Value *stdlib_method(Value *obj, const char *m, Value **a, int argc, Interp *I) {
    /* Text methods */
    if (obj->kind==V_TEXT) {
        const char *s=obj->sval; int slen=(int)strlen(s);
        if (!strcmp(m,"upper"))  { char *b=strdup(s); for(int i=0;b[i];i++) b[i]=toupper((u8)b[i]); Value *r=v_text(b); free(b); return r; }
        if (!strcmp(m,"lower"))  { char *b=strdup(s); for(int i=0;b[i];i++) b[i]=tolower((u8)b[i]); Value *r=v_text(b); free(b); return r; }
        if (!strcmp(m,"trim"))   { const char *st=s; while(*st&&isspace((u8)*st))st++; const char *en=s+slen-1; while(en>st&&isspace((u8)*en))en--; int n=(int)(en-st+1); char *b=malloc(n+1); strncpy(b,st,n); b[n]=0; Value *r=v_text(b); free(b); return r; }
        if (!strcmp(m,"len")||!strcmp(m,"count")) return v_int((i64)slen);
        if (!strcmp(m,"reverse")) { char *b=malloc(slen+1); for(int i=0;i<slen;i++) b[i]=s[slen-1-i]; b[slen]=0; Value *r=v_text(b); free(b); return r; }
        if (!strcmp(m,"contains")) { if(!argc||a[0]->kind!=V_TEXT)return v_bool(0); return v_bool(strstr(s,a[0]->sval)!=NULL); }
        if (!strcmp(m,"startsWith")) { if(!argc||a[0]->kind!=V_TEXT)return v_bool(0); return v_bool(strncmp(s,a[0]->sval,strlen(a[0]->sval))==0); }
        if (!strcmp(m,"endsWith")) { if(!argc||a[0]->kind!=V_TEXT)return v_bool(0); int plen=(int)strlen(a[0]->sval); if(plen>slen)return v_bool(0); return v_bool(strcmp(s+slen-plen,a[0]->sval)==0); }
        if (!strcmp(m,"replace")) {
            if(argc<2||a[0]->kind!=V_TEXT||a[1]->kind!=V_TEXT){vel_err(I,0,"replace() needs 2 Text args");return v_nil();}
            const char *fr=a[0]->sval, *to=a[1]->sval; int flen=(int)strlen(fr), tlen=(int)strlen(to);
            char *buf=malloc(MAX_STR); int bi=0; const char *cur=s;
            while(*cur&&bi<MAX_STR-1){ if(flen&&strncmp(cur,fr,flen)==0){ int copy=tlen<MAX_STR-bi-1?tlen:MAX_STR-bi-1; memcpy(buf+bi,to,copy); bi+=copy; cur+=flen; } else buf[bi++]=*cur++; }
            buf[bi]=0; Value *r=v_text(buf); free(buf); return r;
        }
        if (!strcmp(m,"split")) { Value *av[2]={obj,argc?a[0]:NULL}; return stdlib_call("split",av,argc?2:1,I); }
        if (!strcmp(m,"after")) {
            if(!argc||a[0]->kind!=V_TEXT) return v_text("");
            const char *f=strstr(s,a[0]->sval); return f ? v_text(f+strlen(a[0]->sval)) : v_text("");
        }
        if (!strcmp(m,"toJson")) { char buf[MAX_STR]; snprintf(buf,sizeof(buf),"\"%s\"",s); return v_text(buf); }
    }
    /* List methods */
    if (obj->kind==V_LIST) {
        int n=obj->list.len;
        if (!strcmp(m,"count")||!strcmp(m,"len")) return v_int((i64)n);
        if (!strcmp(m,"first")) return n?v_some(obj->list.items[0]):v_none();
        if (!strcmp(m,"last"))  return n?v_some(obj->list.items[n-1]):v_none();
        if (!strcmp(m,"reverse")) { Value *r=v_list(); for(int i=n-1;i>=0;i--) v_list_push(r,obj->list.items[i]); return r; }
        if (!strcmp(m,"contains")) { if(!argc)return v_bool(0); for(int i=0;i<n;i++) if(v_equal(obj->list.items[i],a[0])) return v_bool(1); return v_bool(0); }
        if (!strcmp(m,"sum"))  { Value *av[1]={obj}; return stdlib_call("sum",av,1,I); }
        if (!strcmp(m,"join")) { Value *av[2]={obj,argc?a[0]:NULL}; return stdlib_call("join",av,argc?2:1,I); }
        if (!strcmp(m,"sort")) {
            Value *r=v_list(); for(int i=0;i<n;i++) v_list_push(r,obj->list.items[i]);
            /* insertion sort */
            for(int i=1;i<r->list.len;i++){
                Value *key=r->list.items[i]; int j=i-1;
                while(j>=0){
                    Value *x=r->list.items[j]; int gt=0;
                    if(x->kind==V_INT&&key->kind==V_INT) gt=x->ival>key->ival;
                    else if(x->kind==V_FLOAT&&key->kind==V_FLOAT) gt=x->fval>key->fval;
                    else if(x->kind==V_TEXT&&key->kind==V_TEXT) gt=strcmp(x->sval,key->sval)>0;
                    if(!gt) { break; }
                    r->list.items[j+1]=r->list.items[j]; j--;
                }
                r->list.items[j+1]=key;
            }
            return r;
        }
        if (!strcmp(m,"filter")) {
            if(!argc||a[0]->kind!=V_FN){vel_err(I,0,"filter() needs a function");return v_nil();}
            Value *r=v_list();
            for(int i=0;i<n;i++){ Value *item=obj->list.items[i]; Value *res=call_fn(I,&a[0]->fn,&item,1); if(v_truthy(res)) v_list_push(r,item); if(I->had_err)return v_nil(); }
            return r;
        }
        if (!strcmp(m,"map")) {
            if(!argc||a[0]->kind!=V_FN){vel_err(I,0,"map() needs a function");return v_nil();}
            Value *r=v_list();
            for(int i=0;i<n;i++){
                Value *item=obj->list.items[i];
                Value *res=call_fn(I,&a[0]->fn,&item,1);
                if(I->had_err) return v_nil();
                v_list_push(r,res);
            }
            return r;
        }
        if (!strcmp(m,"reduce")) {
            if(argc<2||a[0]->kind!=V_FN){vel_err(I,0,"reduce() needs fn and init");return v_nil();}
            Value *acc=a[1];
            for(int i=0;i<n;i++){ Value *pair[2]={acc,obj->list.items[i]}; acc=call_fn(I,&a[0]->fn,pair,2); if(I->had_err)return v_nil(); }
            return acc;
        }
        if (!strcmp(m,"toJson")) { char buf[MAX_STR]; int off=snprintf(buf,sizeof(buf),"["); for(int i=0;i<n&&off<(int)sizeof(buf)-8;i++){char tmp[256];v_tostr(obj->list.items[i],tmp,sizeof(tmp)); off+=snprintf(buf+off,sizeof(buf)-off,obj->list.items[i]->kind==V_TEXT?"\"%s\"%s":"%s%s",tmp,i<n-1?", ":"");} snprintf(buf+off,sizeof(buf)-off,"]"); return v_text(buf); }
    }
    /* Map methods */
    if (obj->kind==V_MAP) {
        if (!strcmp(m,"count")||!strcmp(m,"len")) return v_int((i64)obj->map.len);
        if (!strcmp(m,"keys"))   { Value *r=v_list(); for(int i=0;i<obj->map.len;i++) v_list_push(r,obj->map.keys[i]); return r; }
        if (!strcmp(m,"values")) { Value *r=v_list(); for(int i=0;i<obj->map.len;i++) v_list_push(r,obj->map.vals[i]); return r; }
        if (!strcmp(m,"has")) { if(!argc)return v_bool(0); for(int i=0;i<obj->map.len;i++) if(v_equal(obj->map.keys[i],a[0]))return v_bool(1); return v_bool(0); }
    }
    /* Result/Option */
    if (obj->kind==V_RESULT) {
        if (!strcmp(m,"unwrap")) { if(!obj->res.is_ok){vel_err(I,0,"Unwrapped fail: %s",obj->res.err);return v_nil();} return obj->res.inner; }
    }
    if (obj->kind==V_OPTION) {
        if (!strcmp(m,"unwrap")) { if(!obj->opt.is_some){vel_err(I,0,"Unwrapped none");return v_nil();} return obj->opt.inner; }
    }
    vel_err(I,0,"No method '%s' on %s", m, v_typename(obj)); return v_nil();
}
