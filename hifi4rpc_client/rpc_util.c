#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "aipc_type.h"

/**
#define STRING_RAW_MAX_LEN 128
typedef struct {
    int32_t len;
    int8_t raw[STRING_RAW_MAX_LEN];
} raw_st;
*/

#define DELIM     ' '

int from_arg(int argc, char **argv, raw_st *p) {
    int i, len, cap;
    for (i = 0, len = 0, cap = STRING_RAW_MAX_LEN; i != argc; i++) {
        len += snprintf(p->raw + len, cap - len, "%s%c", argv[i], DELIM);
    }
    p->raw[len - 1] = '\0'; // change final whitespace to \0
    p->len = len;
    return 0;
}

/** strndup come from glibc, toolchain may not include it */
static char *my_strndup(const char *s, size_t n) {
    char *r = malloc(sizeof(char) * n);
    strncpy(r, s, n);
    return r;
}

int to_arg(raw_st *p, int *o_argc, char ***o_argv) {
    char *r = p->raw;
    char *e;
    int c = 0;
    char **v = NULL;
    do {
        e = strchr(r, DELIM);
        c++;
        v = realloc(v, sizeof(char *) * c);
        v[c - 1] = my_strndup(r, e - r);
        r = e + 1; // pass the delim
    } while (e != NULL);
    *o_argc = c;
    *o_argv = v;
    return 0;
}

int append_arg(int *o_argc, char ***o_argv, char *s) {
    int c = *o_argc;
    char **v = *o_argv;
    c++;
    v = realloc(v, sizeof(char *) * c);
    v[c - 1] = strdup(s);
    *o_argc = c;
    *o_argv = v;
    return 0;
}

void free_arg(int argc, char **argv) {
    int i;
    for (i = 0; i != argc; i++) {
        free(argv[i]);
    }
    free(argv);
}

void show_arg(int argc, char **argv) {
    int i;
    for (i = 0; i != argc; i++) {
        printf("%d:%s\n", i, argv[i]);
    }
}


/** self test
static void showraw(raw_st *p) {
    printf("raw_st=%p len=%d raw='%s'\n", p, p->len, p->raw);
}

int main() {
    char *argv[] = { "hello", "world", "dumb", "test"};
    int argc = sizeof(argv) / sizeof(argv[0]);
    raw_st arg;
    arg2raw(&arg, argc, argv);
    showraw(&arg);

    int c;
    char **v;
    raw2arg(&arg, &c, &v);
    showarg(c, v);
    freearg(c, v);
    return 0;
}
*/
