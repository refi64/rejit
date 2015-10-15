#define REJIT_INSTR
#include "rejit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

rejit_parse_result rejit_parse(const char* str, int* err) {
    rejit_parse_result res;
    // XXX: This is a rough estimate that may waste memory!
    size_t instc=0;
    res.instrs = malloc(strlen(str));
    if (!res.instrs) { puts("memory error"); abort(); }
    res.groups = 0;
    // XXX: This order is insanely important!
    const char special[] = "^$^*+?";
    #define SPECIAL(c) (memchr(special, sizeof(special)/sizeof(char), (c)))
    while (*str) {
        size_t word=0, i;
        for (;;) {
            if (str[word] == '\\') {
                word += 2;
                if (!word) { *err = 1; return res; }
            }
            else if (SPECIAL(str[word])) break;
            else ++word;
        }
        // word now points to the length of the current text.
        if (SPECIAL(str[word])) ++instc;
        /*     res.instrs[instc++] = {IBEGIN+(special-SPECIAL(str[word]))}; */
        for (i=0; i < word; ++i) {
            res.instrs[instc].kind = ICHR;
            res.instrs[instc].value = 0;
            ++instc;
        }
    }
    *err = 0;
    return res;
}

void rejit_free_parse_result(rejit_parse_result p) { free(p.instrs); }
