#ifndef REJIT_H
#define REJIT_H

#include <inttypes.h>
#include <string.h>

typedef long (*rejit_func)(const char*);

typedef struct rejit_matcher_type {
    rejit_func func;
    size_t sz;
    int groups;
}* rejit_matcher;

typedef enum {
    RJ_INULL, RJ_ICHR, RJ_IDOT, RJ_IBEGIN, RJ_IEND,
    RJ_ISET, RJ_IARG, RJ_ISTAR, RJ_IPLUS, RJ_IOPT, RJ_IMSTAR, RJ_IMPLUS, RJ_IVARG,
    RJ_IOR, RJ_IGROUP, RJ_ISKIP
    // > iarg: following op is argument.
    // > varg: value is rejit_instruction*.
    // For RJ_IGROUP, value points to one past the end of the current group.
    // For RJ_ISET, value is const char*.
} rejit_instr_kind;

typedef struct rejit_instruction_type {
    rejit_instr_kind kind;
    intptr_t value;
} rejit_instruction;

typedef struct rejit_token_type {
    enum {
        RJ_TWORD, RJ_TCARET, RJ_TDOLLAR, RJ_TDOT, RJ_TLP, RJ_TRP, RJ_TLK, RJ_TRK,
        RJ_TSUF,
        RJ_TSTAR, RJ_TPLUS, RJ_TQ,
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
} rejit_parse_result;

typedef struct rejit_parse_error_type {
    enum {
        RJ_PE_NONE,   // Successful parse/no error.
        RJ_PE_SYNTAX, // Syntax error.
        RJ_PE_UBOUND, // Unbound parenthesis.
        RJ_PE_OVFLOW, // Stack overflow (too many nested parens).
        RJ_PE_MEM,    // Out of memory.
    } kind;
    size_t pos;
} rejit_parse_error;

rejit_token_list rejit_tokenize(const char* str, rejit_parse_error* err);
void rejit_free_tokens(rejit_token_list tokens);
rejit_parse_result rejit_parse(const char* str, rejit_parse_error* err);
void rejit_free_parse_result(rejit_parse_result res);
rejit_matcher rejit_compile_instrs(rejit_instruction* instrs, int groups);
int rejit_match(rejit_matcher m, const char* str);
int rejit_search(rejit_matcher m, const char* str, const char** tgt);
void rejit_free_matcher(rejit_matcher m);

#endif
