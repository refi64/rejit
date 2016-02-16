/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include <stdlib.h>
#include <stdio.h>

#include "rejit.h"

#define ERR(...) fprintf(stderr, __VA_ARGS__)

rejit_matcher compile(char* regex) {
    rejit_parse_error err;
    rejit_matcher m = rejit_parse_compile(regex, &err, RJ_FNONE);
    if (err.kind != RJ_PE_NONE) {
        ERR("parse error at %zu: ", err.pos);
        switch (err.kind) {
        case RJ_PE_NONE: abort();
        case RJ_PE_SYNTAX: ERR("invalid syntax"); break;
        case RJ_PE_UBOUND: ERR("unbound parenthesis/bracket/curly brace"); break;
        case RJ_PE_OVFLOW: ERR("too many nested parentheses"); break;
        case RJ_PE_RANGE: ERR("invalid character range"); break;
        case RJ_PE_INT: ERR("expected integer"); break;
        case RJ_PE_LBVAR: ERR("lookbehind cannot be variable-length"); break;
        case RJ_PE_MEM: ERR("out of memory"); break;
        }
        ERR("\n");
        return NULL;
    }
    return m;
}

int run_regex(rejit_matcher m, char* string) {
    rejit_group* groups = NULL;
    int len;
    if (m->groups && (groups = calloc(m->groups, sizeof(rejit_group))) == NULL) {
        ERR("out of memory\n");
        return 1;
    }
    len = rejit_match(m, string, groups);
    if (len == -1) {
        ERR("string did not match regex\n");
        free(groups);
        return 1;
    } else {
        int i;
        printf("match successful! length: %d\n", len);
        if (m->groups)
            for (i=0; i<m->groups; ++i) {
                printf("group %d: ", i);
                if (groups[i].begin == NULL) puts("unmatched");
                else
                    printf("\"%.*s\"\n", (int)(groups[i].end-groups[i].begin),
                           groups[i].begin);
            }
        free(groups);
        return 0;
    }
}

int main(int argc, char** argv) {
    rejit_matcher m;
    int res;

    if (argc != 3) {
        ERR("usage: %s <regex> <string>\n", argv[0]);
        return 1;
    }

    if (!(m = compile(argv[1]))) return 1;
    res = run_regex(m, argv[2]);
    rejit_free_matcher(m);
    return res;
}
