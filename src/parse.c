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

rejit_token_list rejit_tokenize(const char* str, rejit_parse_error* err) {
    const char* start = str;
    rejit_token_list tokens;
    int escaped = 0, len = 1;
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
        K('|', P)
        K('(', LP)
        K(')', RP)
        case '[':
            tkind = RJ_TSET;
            if (str[len] == '^') ++str;
            while (str[len] && str[len] != ']') ++len;
            if (!str[len]) {
                err->kind = RJ_PE_UBOUND;
                err->pos = str-start;
                return tokens;
            } else ++len;
            break;
        case '\\': escaped = 1; break;
        default: break;
        }

        token.kind = tkind;
        token.pos = str;
        token.len = len;
        str += len;
        len = 1;

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

typedef struct pipe_type {
    long mid, end;
    rejit_instruction* instr;
} pipe;

static void build_suffix_pipe_list(const char* str, rejit_token_list tokens,
                                   long* suffixes, pipe* pipes,
                                   rejit_parse_error* err) {
    size_t i, prev = -1;
    STACK(size_t) st, pst; // Group, pipe stack.
    st.len = pst.len = 0;

    for (i=0; i<tokens.len; ++i) suffixes[i] = pipes[i].mid = pipes[i].end = -1;

    for (i=0; i<tokens.len; ++i) {
        rejit_token t = tokens.tokens[i];
        if (t.kind == RJ_TLP) {
            PUSH(st, i);
            prev = -1;
        }
        else if (t.kind == RJ_TRP) {
            prev = POP(st);
            if (pst.len) pipes[POP(pst)].end = i;
        }
        else if (t.kind > RJ_TSUF) {
            if (prev == -1) {
                if (t.kind == RJ_TQ) continue;
                err->kind = RJ_PE_SYNTAX;
                err->pos = t.pos - str;
                return;
            }
            suffixes[prev] = i;
            prev = -1;
        } else if (t.kind == RJ_TP) {
            size_t pp;
            if (i+1 == tokens.len) {
                err->kind = RJ_PE_SYNTAX;
                err->pos = t.pos - str;
                return;
            }
            pp = st.len == 0 ? 0 : TOS(st)+1;
            pipes[pp].mid = i+1;
            PUSH(pst, pp);
            prev = -1;
        } else prev = i;
    }
}

static void parse(const char* str, rejit_token_list tokens, long* suffixes,
                  pipe* pipes, rejit_parse_result* res, rejit_parse_error* err) {
    size_t i, ninstrs = 0, sl;
    STACK(rejit_instruction*) st;
    STACK(pipe) pst;
    char* s;
    st.len = pst.len = 0;
    sl = strlen(str);
    ALLOC(res->instrs, sizeof(rejit_instruction)*(tokens.len+1), {
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
        } else if (pst.len && i == TOS(pst).mid)
            TOS(pst).instr->value = (intptr_t)&CUR;
        else if (pst.len && i == TOS(pst).end)
            POP(pst).instr->value2 = (intptr_t)&CUR;

        if (pipes[i].mid != -1) {
            CUR.kind = RJ_IOR;
            pipes[i].instr = &CUR;
            PUSH(pst, pipes[i]);
            ++ninstrs;
        }

        switch (t.kind) {
        case RJ_TWORD:
            CUR.kind = RJ_IWORD;
            ALLOC(s, t.len+1, {
                err->kind = RJ_PE_MEM;
                err->pos = t.pos - str;
                return;
            });
            memcpy(s, t.pos, t.len);
            s[t.len] = 0;
            CUR.value = (intptr_t)s;
            ++ninstrs;
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
            #define M(c,t)\
            if (i+2 < tokens.len && tokens.tokens[i+1].kind == RJ_TQ &&\
                tokens.tokens[i+2].kind == RJ_TWORD &&\
                *tokens.tokens[i+2].pos == c) {\
                CUR.kind = RJ_I##t;\
                ++i;\
                ++tokens.tokens[i+1].pos;\
                --tokens.tokens[i+1].len;\
            }
            M(':', GROUP)
            else M('=', LAHEAD)
            else M('!', NLAHEAD)
            else {
                CUR.kind = RJ_ICGROUP;
                CUR.value2 = res->groups++;
            }
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
        case RJ_TSET:
            CUR.kind = *t.pos == '^' ? RJ_INSET : RJ_ISET;
            ALLOC(s, t.len*2, {
                err->kind = RJ_PE_MEM;
                err->pos = t.pos - str;
                return;
            });
            memcpy(s, t.pos+1, t.len-2);
            s[t.len-2] = 0;
            memset(s+t.len-1, ' ', t.len-2);
            s[t.len*2-3] = 0;
            CUR.value = (intptr_t)s;
            ++ninstrs;
            break;
        default:
            assert(t.kind >= RJ_TP);
        }
    }

    CUR.kind = RJ_INULL;

    if (st.len != 0) {
        err->kind = RJ_PE_UBOUND;
        err->pos = sl;
    }

    while (pst.len) {
        assert(TOS(pst).end == -1);
        POP(pst).instr->value2 = (intptr_t)&CUR;
    }
}

rejit_parse_result rejit_parse(const char* str, rejit_parse_error* err) {
    long* suffixes;
    pipe* pipes;

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
    ALLOC(pipes, sizeof(pipe)*tokens.len, {
        err->kind = RJ_PE_MEM;
        err->pos = 0;
        return res;
    });
    build_suffix_pipe_list(str, tokens, suffixes, pipes, err);
    if (err->kind != RJ_PE_NONE) return res;

    parse(str, tokens, suffixes, pipes, &res, err);
    free(suffixes);
    free(pipes);
    rejit_free_tokens(tokens);
    return res;
}

void rejit_free_parse_result(rejit_parse_result p) {
    int i;
    for (i=0; p.instrs[i].kind != RJ_INULL; ++i) {
        if (p.instrs[i].kind > RJ_ISKIP) p.instrs[i].kind -= RJ_ISKIP;
        if (p.instrs[i].kind == RJ_ISET || p.instrs[i].kind == RJ_IWORD)
            free((void*)p.instrs[i].value);
    }
    free(p.instrs);
}
