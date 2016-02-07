#define REJIT_INSTR
#include "rejit.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define ALLOC(tgt,sz,f) do {\
    (tgt) = calloc(1, sz);\
    if ((tgt) == NULL) f;\
} while (0)

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
        K('^', CARET)
        K('$', DOLLAR)
        K('.', DOT)
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

void rejit_free_tokens(rejit_token_list tokens) { free(tokens.tokens); }

#define MAXSTACK 256

#define STACK(t) struct {\
    t stack[MAXSTACK];\
    size_t len;\
}

#define PUSH(st,t) do {\
    if (st.len+1 >= MAXSTACK) {\
        err->kind = RJ_PE_OVFLOW;\
        return;\
    };\
    st.stack[st.len++] = t;\
} while (0)
#define POP(st) (st.stack[--st.len])
#define TOS(st) (st.stack[st.len-1])

static void build_suffix_list(const char* str, rejit_token_list tokens,
                              long* suffixes, E* err) {
    size_t i, prev = -1;
    STACK(size_t) st;
    st.len = 0;

    for (i=0; i<tokens.len; ++i) suffixes[i] = -1;

    for (i=0; i<tokens.len; ++i) {
        rejit_token t = tokens.tokens[i];
        if (t.kind == RJ_TLP) PUSH(st, i);
        else if (t.kind == RJ_TRP) prev = POP(st);
        else if (t.kind > RJ_TSUF) {
            if (prev == -1) {
                if (t.kind == RJ_TQ) continue;
                err->kind = RJ_PE_SYNTAX;
                err->pos = t.pos - str;
                return;
            }
            suffixes[prev] = i;
            prev = -1;
        } else prev = i;
    }
}

static void parse(const char* str, rejit_token_list tokens, long* suffixes,
                  rejit_parse_result* res, E* err) {
    size_t i, j, ninstrs = 0, sl;
    STACK(rejit_instruction*) st;
    st.len = 0;
    sl = strlen(str);
    ALLOC(res->instrs, sizeof(rejit_instruction)*(sl+1), {
        err->kind = RJ_PE_MEM;
        err->pos = 0;
        return;
    });

    #define CUR res->instrs[ninstrs]

    for (i=0; i<tokens.len; ++i) {
        rejit_token t = tokens.tokens[i];

        if (suffixes[i] != -1) {
            CUR.kind = tokens.tokens[suffixes[i]].kind - RJ_TSTAR + RJ_ISTAR;
            if (suffixes[i]+1 < tokens.len &&
                tokens.tokens[suffixes[i]+1].kind == RJ_TQ && CUR.kind != RJ_IOPT)
                CUR.kind += RJ_IMSTAR - RJ_ISTAR;
            ++ninstrs;
        }

        switch (t.kind) {
        case RJ_TWORD:
            for (j=0; j<t.len; ++j) {
                CUR.kind = RJ_ICHR;
                CUR.value = t.pos[j];
                ++ninstrs;
            }
            break;
        case RJ_TCARET: case RJ_TDOLLAR:
            CUR.kind = RJ_IEND - (RJ_TDOLLAR - t.kind);
            ++ninstrs;
            break;
        case RJ_TDOT:
            CUR.kind = RJ_IDOT;
            ++ninstrs;
            break;
        case RJ_TLP:
            CUR.kind = RJ_IGROUP;
            PUSH(st, &CUR);
            ++ninstrs;
            break;
        case RJ_TRP:
            if (st.len == 0) {
                err->kind = RJ_PE_UBOUND;
                err->pos = t.pos - str;
                return;
            }
            POP(st)->value = (intptr_t)&CUR;
            break;
        default:
            assert(t.kind > RJ_TSUF);
        }
    }

    CUR.kind = RJ_INULL;

    if (st.len != 0) {
        err->kind = RJ_PE_UBOUND;
        err->pos = sl;
    }
}

rejit_parse_result rejit_parse(const char* str, E* err) {
    STACK(size_t) st;
    st.len = 0;
    long* suffixes;

    rejit_parse_result res;
    rejit_token_list tokens;
    res.instrs = NULL;
    res.groups = 0;

    err->kind = RJ_PE_NONE;
    err->pos = 0;

    tokens = rejit_tokenize(str, err);
    if (err->kind != RJ_PE_NONE) return res;

    ALLOC(suffixes, sizeof(long)*tokens.len, {
        err->kind = RJ_PE_MEM;
        err->pos = 0;
        return res;
    });
    build_suffix_list(str, tokens, suffixes, err);
    if (err->kind != RJ_PE_NONE) return res;

    parse(str, tokens, suffixes, &res, err);
    return res;
}

void rejit_free_parse_result(rejit_parse_result p) { free(p.instrs); }
