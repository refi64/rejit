/* This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef REJIT_H
#define REJIT_H

#include <inttypes.h>
#include <string.h>

typedef struct rejit_group_type {
    const char* begin, *end;
} rejit_group;

typedef long (*rejit_func)(const char*, rejit_group*);

typedef struct rejit_matcher_type {
    rejit_func func;
    size_t sz;
    int groups;
}* rejit_matcher;

typedef enum {
    RJ_INULL, RJ_IWORD, RJ_IDOT, RJ_IBEGIN, RJ_IEND, RJ_IBACK,
    RJ_ISET, RJ_INSET, RJ_IARG, RJ_ISTAR, RJ_IPLUS, RJ_IOPT, RJ_IREP, RJ_IMSTAR,
    RJ_IMPLUS, RJ_IVARG, RJ_IOR, RJ_IGROUP, RJ_ICGROUP, RJ_ILAHEAD, RJ_INLAHEAD,
    RJ_ILBEHIND, RJ_ISKIP
    /* > iarg: following op is argument.
       > varg: value is rejit_instruction*.
       For RJ_IGROUP, RJ_ICGROUP, value points to one past the end of the current
       group. For RJ_ISET, value is const char*. */
} rejit_instr_kind;

typedef enum {
    RJ_FNONE   = 1<<0,
    RJ_FICASE  = 1<<1,
    RJ_FDOTALL = 1<<2,
} rejit_flags;

typedef struct rejit_instruction_type {
    rejit_instr_kind kind;
    intptr_t value, value2;
    size_t len;
} rejit_instruction;

typedef struct rejit_token_type {
    enum {
        RJ_TWORD, RJ_TCARET, RJ_TDOLLAR, RJ_TDOT, RJ_TLP, RJ_TRP, RJ_TSET, RJ_TMS,
        RJ_TBACK, RJ_TP,
        RJ_TSUF,
        RJ_TSTAR, RJ_TPLUS, RJ_TQ, RJ_TREP,
    } kind;
    const char* pos;
    size_t len;
} rejit_token;

typedef struct rejit_token_list_type {
    rejit_token* tokens;
    size_t len;
} rejit_token_list;

typedef struct rejit_parse_result_type {
    rejit_instruction* instrs;
    int groups;
    rejit_flags flags;
} rejit_parse_result;

typedef struct rejit_parse_error_type {
    enum {
        RJ_PE_NONE,   // Successful parse/no error.
        RJ_PE_SYNTAX, // Syntax error.
        RJ_PE_UBOUND, // Unbound parenthesis, bracket, or curly brace.
        RJ_PE_OVFLOW, // Stack overflow (too many nested parens).
        RJ_PE_RANGE,  // Bad character range.
        RJ_PE_INT,    // Expected an integer.
        RJ_PE_MEM,    // Out of memory.
    } kind;
    size_t pos;
} rejit_parse_error;

rejit_token_list rejit_tokenize(const char* str, rejit_parse_error* err);
void rejit_free_tokens(rejit_token_list tokens);
rejit_parse_result rejit_parse(const char* str, rejit_parse_error* err);
void rejit_free_parse_result(rejit_parse_result res);
int rejit_match_len(rejit_instruction* instr);
rejit_matcher rejit_compile_instrs(rejit_instruction* instrs, int groups,
                                   rejit_flags flags);
rejit_matcher rejit_compile(rejit_parse_result res, rejit_flags flags);
rejit_matcher rejit_parse_compile(const char* str, rejit_parse_error* err,
                                  rejit_flags flags);
int rejit_match(rejit_matcher m, const char* str, rejit_group* groups);
int rejit_search(rejit_matcher m, const char* str, const char** tgt,
                 rejit_group* groups);
void rejit_free_matcher(rejit_matcher m);

#endif
