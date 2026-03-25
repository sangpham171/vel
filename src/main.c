/*
 * main.c — VEL language CLI
 * Build: gcc -std=c99 -O2 -o vel src/main.c src/util.c src/lexer.c \
 *        src/parser.c src/interpreter.c src/env.c src/value.c \
 *        src/stdlib.c src/error.c -lm
 */
#include "vel.h"

#define VEL_VERSION "0.1.0"

static char *read_file(const char *path) {
    FILE *f = fopen(path, "r");
    if (!f) { fprintf(stderr, "\033[31merror:\033[0m Cannot open: %s\n", path); return NULL; }
    fseek(f, 0, SEEK_END); long sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz > SRC_MAX) { fprintf(stderr, "\033[31merror:\033[0m File too large\n"); fclose(f); return NULL; }
    char *buf = malloc(sz + 1);
    if (fread(buf, 1, sz, f) != (size_t)sz && !feof(f)) { fclose(f); free(buf); return NULL; }
    buf[sz] = '\0'; fclose(f); return buf;
}

static int do_run(const char *src) {
    Token *toks = calloc(MAX_TOKENS, sizeof(Token));
    int n = vel_lex(src, toks, MAX_TOKENS);
    if (n < 0) { free(toks); return 1; }
    Node *prog = vel_parse(toks, n);
    if (!prog) { free(toks); return 1; }
    Interp I; interp_init(&I);
    Env *g = env_new(NULL);
    stdlib_register(g);
    interp_run(&I, prog, g);
    free(toks);
    return I.had_err ? 1 : 0;
}

static int do_lex(const char *src) {
    Token *toks = calloc(MAX_TOKENS, sizeof(Token));
    int n = vel_lex(src, toks, MAX_TOKENS);
    if (n < 0) { free(toks); return 1; }
    const char *names[T_COUNT];
    memset(names, 0, sizeof(names));
    names[T_INT]="INT"; names[T_FLOAT]="FLOAT"; names[T_STR]="STR";
    names[T_TRUE]="TRUE"; names[T_FALSE]="FALSE"; names[T_NONE]="NONE";
    names[T_IDENT]="IDENT"; names[T_LET]="LET"; names[T_FIX]="FIX";
    names[T_FN]="FN"; names[T_CLASS]="CLASS"; names[T_IF]="IF";
    names[T_ELSE]="ELSE"; names[T_MATCH]="MATCH"; names[T_FOR]="FOR";
    names[T_WHILE]="WHILE"; names[T_REPEAT]="REPEAT"; names[T_SAY]="SAY";
    names[T_LOG]="LOG"; names[T_ARROW]="ARROW"; names[T_EQ]="EQ";
    names[T_EQEQ]="EQEQ"; names[T_NL]="NL"; names[T_INDENT]="INDENT";
    names[T_DEDENT]="DEDENT"; names[T_EOF]="EOF";
    for (int i = 0; i < n; i++) {
        const char *nm = names[toks[i].kind] ? names[toks[i].kind] : "?";
        printf("  [%3d] %-10s  %s\n", toks[i].line, nm, toks[i].lex);
    }
    free(toks); return 0;
}

static int do_check(const char *src) {
    Token *toks = calloc(MAX_TOKENS, sizeof(Token));
    int n = vel_lex(src, toks, MAX_TOKENS);
    if (n < 0) { free(toks); return 1; }
    Node *prog = vel_parse(toks, n);
    free(toks);
    if (!prog) return 1;
    printf("\033[32m✓\033[0m No parse errors\n");
    return 0;
}

static void repl(void) {
    printf("\033[35;1m"
           "   _   ___ ___  _  _ \n"
           "  /_\\ | __/ _ \\| \\| |\n"
           " / _ \\| _| (_) | .` |\n"
           "/_/ \\_|___\\___/|_|\\_|\n"
           "\033[0m\033[90mVEL %s — REPL  (.exit to quit)\033[0m\n\n", VEL_VERSION);

    Interp I; interp_init(&I);
    Env *g = env_new(NULL); stdlib_register(g);
    char line[4096];

    while (1) {
        printf("\033[35mvel>\033[0m "); fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) { printf("\n"); break; }
        int len = (int)strlen(line);
        while (len > 0 && (line[len-1]=='\n'||line[len-1]=='\r')) line[--len]='\0';
        if (!len) continue;
        if (!strcmp(line,".exit")||!strcmp(line,".quit")) { printf("\033[35mGoodbye!\033[0m\n"); break; }
        if (!strcmp(line,".help")) { printf("  \033[36m.exit\033[0m  quit\n  \033[36m.help\033[0m  this message\n\n"); continue; }

        Token *toks = calloc(MAX_TOKENS, sizeof(Token));
        int n = vel_lex(line, toks, MAX_TOKENS);
        if (n < 0) { free(toks); continue; }
        Node *prog = vel_parse(toks, n);
        if (!prog) { free(toks); continue; }
        I.had_err=0; I.sig=SIG_NONE; I.retval=NULL;
        Value *r = interp_run(&I, prog, g);
        if (!I.had_err && r && r->kind != V_NIL) {
            printf("\033[36m=>\033[0m "); v_print(r, stdout); printf("\n");
        }
        free(toks);
    }
}

static void help(void) {
    printf("\033[35;1mVEL\033[0m %s — The language built for humans and AI\n\n", VEL_VERSION);
    printf("Usage: vel <command> [file.vel]\n\n");
    printf("  \033[36mrun\033[0m   <file>   Run program\n");
    printf("  \033[36mcheck\033[0m <file>   Parse-check only\n");
    printf("  \033[36mlex\033[0m   <file>   Dump tokens\n");
    printf("  \033[36mrepl\033[0m           Interactive REPL\n");
    printf("  \033[36mversion\033[0m        Show version\n\n");
    printf("Build:  gcc -std=c99 -O2 -o vel src/*.c -lm\n");
}

int main(int argc, char **argv) {
    if (argc < 2) { help(); return 0; }
    const char *cmd = argv[1];

    if (!strcmp(cmd,"version")||!strcmp(cmd,"--version")||!strcmp(cmd,"-v"))
        { printf("vel %s\n", VEL_VERSION); return 0; }
    if (!strcmp(cmd,"help")||!strcmp(cmd,"--help")||!strcmp(cmd,"-h"))
        { help(); return 0; }
    if (!strcmp(cmd,"repl")) { repl(); return 0; }

    if (argc < 3) { fprintf(stderr,"\033[31merror:\033[0m Missing file\n"); return 1; }
    char *src = read_file(argv[2]);
    if (!src) return 1;

    int rc = 0;
    if      (!strcmp(cmd,"run"))   rc = do_run(src);
    else if (!strcmp(cmd,"check")) rc = do_check(src);
    else if (!strcmp(cmd,"lex"))   rc = do_lex(src);
    else { fprintf(stderr,"\033[31merror:\033[0m Unknown command '%s'\n",cmd); rc=1; }

    free(src); return rc;
}
