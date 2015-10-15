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

#ifdef REJIT_INSTR
typedef enum {
    INULL, ICHR, IDOT, IBEGIN, IEND,
    ISET, IARG, ISTAR, IPLUS, IOPT, IVARG, IOR,
    ISKIP
    // > iarg: following op is argument.
    // > varg: value is rejit_instruction*.
    // For ISET, value is const char*.
} rejit_instr_kind;
#else
typedef int rejit_instr_kind;
#endif

typedef struct rejit_instruction_type {
    rejit_instr_kind kind;
    intptr_t value;
} rejit_instruction;

typedef struct rejit_parse_result_type {
    rejit_instruction* instrs;
    int groups;
} rejit_parse_result;

rejit_parse_result rejit_parse(const char* str, int* err);
void rejit_free_parse_result(rejit_parse_result res);
rejit_matcher rejit_compile_instrs(rejit_instruction* instrs, int groups);
int rejit_match(rejit_matcher m, const char* str);
void rejit_free_matcher(rejit_matcher m);

#endif
