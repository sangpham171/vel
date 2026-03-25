/* error.c */
#include "vel.h"
void vel_err(Interp *I, int line, const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    vsnprintf(I->err, sizeof(I->err), fmt, ap);
    va_end(ap);
    I->had_err = 1;
    I->sig = SIG_RET;
    if (line > 0) fprintf(stderr, "\033[31merror\033[0m (line %d): %s\n", line, I->err);
    else          fprintf(stderr, "\033[31merror:\033[0m %s\n", I->err);
}
void vel_fatal(const char *fmt, ...) {
    va_list ap; va_start(ap, fmt);
    fprintf(stderr, "\033[31mfatal:\033[0m "); vfprintf(stderr, fmt, ap); fprintf(stderr, "\n");
    va_end(ap); exit(1);
}
