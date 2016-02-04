#define REJIT_INSTR
#include "rejit.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ALLOC(tgt,sz,f) do {\
    (tgt) = malloc(sz);\
    if ((tgt) == NULL) f;\
\} while (0)

#define REALLOC(tgt,sz,f) do {\
    void* realloc_r = realloc((tgt), (sz));\
    if (realloc_r == NULL) {\
        free((tgt));\
        f;\
    } else (tgt) = realloc_r;\
} while (0)

typedef rejit_parse_error E;
typedef rejit_token T;

rejit_token_list rejit_tokenize(const char* str, E* err) {
    const char* start = str;
    rejit_token_list tokens;
    int escaped = 0;
    rejit_token token;

    tokens.tokens = NULL;
    tokens.len = 0;

    while (*str) {
        int tkind = RJ_TWORD;

        if (escaped) escaped = 0;
        else switch (*str) {
        #define K(c,k) case c: tkind = RJ_T##k; break;
        K('+', PLUS)
        K('*', STAR)
        K('?', Q)
        K('(', LP)
        K(')', RP)
        K('[', LK)
        K(']', RK)
        case '\\': escaped = 1; break;
        default: break;
        }

        token.kind = tkind;
        token.pos = str;
        token.len = 1;
        ++str;

        #define PREV (tokens.tokens[tokens.len-1])

        if (token.kind == RJ_TWORD && tokens.tokens && PREV.kind == RJ_TWORD)
            // Merge successive TWORDs.
            ++PREV.len;
        else {
            REALLOC(tokens.tokens, sizeof(rejit_token)*(++tokens.len), {
                err->kind = RJ_PE_MEM;
                err->pos = str-start;
                return tokens;
            });
            PREV = token;
        }
    }

    return tokens;
}

rejit_parse_result rejit_parse(const char* str, E* err) {
    rejit_parse_result res;
    rejit_token_list tokens;
    res.instrs = NULL;
    res.groups = 0;

    err->kind = RJ_PE_NONE;
    err->pos = 0;

    tokens = rejit_tokenize(str, err);
    if (err->kind != RJ_PE_NONE) return res;
    return res;
}

void rejit_free_parse_result(rejit_parse_result p) { free(p.instrs); }
