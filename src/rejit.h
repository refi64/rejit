/* This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef REJIT_H
#define REJIT_H

/*! @header
    @author Ryan Gonzalez
    @unsorted
    @discussion
    This is the documentation for ReJit. Not everything is fully done yet, but
    this should be complete enough to be (somewhat) usable. */

#include <inttypes.h>
#include <string.h>

typedef uint64_t rj_ui64;

/*! @struct rejit_group
    @brief A group.
    @discussion
    This is the type that is used to store groups that were matched in the input.
    Note that both pointers are to the original string. Therefore, the original
    must not be deleted for these pointers to remain valid.

    @field begin A pointer to the beginning of the match.
    @field end A pointer to the end of the match. */
typedef struct rejit_group_type {
    const char* begin, *end;
} rejit_group;

/*! @enum rejit_flags
    @brief Compile flags.
    @discussion
    The different flags you can pass to the <code>*compile*</code> functions.
    Enum values can be bitwise OR'd with other ones to compile various flags.

    @const RJ_FNONE No flags.
    @const RJ_FICASE Case insensitive matching.
    @const RJ_FDOTALL Make dot (<code>.</code>) also match newlines.
    @const RJ_FUNICODE Make character classes Unicode-aware. */
typedef enum {
    RJ_FNONE    = 1<<0,
    RJ_FICASE   = 1<<1,
    RJ_FDOTALL  = 1<<2,
    RJ_FUNICODE = 1<<3,
} rejit_flags;

typedef long (*rejit_func)(const char*, rejit_group*);

/*! @struct rejit_matcher
    @brief A compiled regex.
    @discussion
    This is the type that is returned when compiling a regular expression. All
    the fields should be treated as an internal implementation detail except for
    @link //apple_ref/doc/structfield/rejit_matcher/groups @/link.

    @field groups The number of groups that the compiled regex requires.
    @field flags The flags the regex was compiled with. */
typedef struct rejit_matcher_type {
    rejit_func func;
    size_t sz;
    int groups;
    rejit_flags flags;
}* rejit_matcher;

typedef enum {
    RJ_INULL, RJ_IWORD, RJ_IDOT, RJ_IBEGIN, RJ_IEND, RJ_IBACK,
    RJ_ISET, RJ_INSET, RJ_IUSET, RJ_IARG, RJ_ISTAR, RJ_IPLUS, RJ_IOPT, RJ_IREP,
    RJ_IMSTAR, RJ_IMPLUS, RJ_IVARG, RJ_IOR, RJ_IGROUP, RJ_ICGROUP, RJ_ILAHEAD,
    RJ_INLAHEAD, RJ_ILBEHIND, RJ_INLBEHIND, RJ_ISKIP
    /* > iarg: following op is argument.
       > varg: value is rejit_instruction*.
       For RJ_IGROUP, RJ_ICGROUP, value points to one past the end of the current
       group. For RJ_ISET, value is const char*. */
} rejit_instr_kind;

typedef struct rejit_instruction_type {
    rejit_instr_kind kind;
    intptr_t value, value2;
    size_t len;
    struct rejit_instruction_type* len_from;
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

/*! @struct rejit_parse_result
    @brief The value returned from @link rejit_parse @/link. */
typedef struct rejit_parse_result_type {
    rejit_instruction* instrs;
    int groups, maxdepth;
    rejit_flags flags;
} rejit_parse_result;

/*! @enum rejit_parse_error_kind
    @brief The kind of error that occurred.

    @const RJ_PE_NONE Successful parse/no error.
    @const RJ_PE_UBOUND Unbound parenthesis, bracket, or curly brace.
    @const RJ_PE_OVFLOW Stack overflow (too many nested parens).
    @const RJ_PE_RANGE Bad character range.
    @const RJ_PE_INT Expected an integer.
    @const RJ_PE_LBVAR Lookbehind contains variable-length expression.
    @const RJ_PE_MEM Out of memory. */
typedef enum {
    RJ_PE_NONE,
    RJ_PE_SYNTAX,
    RJ_PE_UBOUND,
    RJ_PE_OVFLOW,
    RJ_PE_RANGE,
    RJ_PE_INT,
    RJ_PE_LBVAR,
    RJ_PE_MEM,
} rejit_parse_error_kind;

/*! @struct rejit_parse_error
    @brief The type that holds parse errors.

    @field kind The kind of error that occurred. If set to
                @link RJ_PE_NONE @/link, then there were no errors during parsing.
    @field pos The position that the error occurred at. */
typedef struct rejit_parse_error_type {
    rejit_parse_error_kind kind;
    size_t pos;
} rejit_parse_error;

rejit_token_list rejit_tokenize(const char* str, rejit_parse_error* err);
void rejit_free_tokens(rejit_token_list tokens);
/*! @function rejit_parse
    @brief Parse a string.
    @discussion
    Parse the given string, returning a @link rejit_parse_result @/link. If an
    error occurs, store the information in the memory pointed to by @link err
    @/link. Otherwise, <code>@link err @/link->@link
    //apple_ref/doc/structfield/rejit_parse_error/kind @/link</code> will be set
    to @link RJ_PE_NONE @/link.

    @param str The string to parse.
    @param err The location to write errors to.
    @param flags The flags to use during parsing. */
rejit_parse_result rejit_parse(const char* str, rejit_parse_error* err,
                               rejit_flags flags);
/*! @function rejit_free_parse_result
    @brief Free the value returned from @link rejit_parse_result @/link. */
void rejit_free_parse_result(rejit_parse_result res);
int rejit_match_len(rejit_instruction* instr);
rejit_matcher rejit_compile_instrs(rejit_instruction* instrs, int groups,
                                   int maxdepth, rejit_flags flags);
/*! @function rejit_compile
    @brief Compile the result of calling @link rejit_parse @/link.

    @param res The value returned from @link rejit_parse @/link.
    @param flags Flags that affect regex compilation. See
                 @link rejit_flags @/link.
    @result A compiled regex matcher. */
rejit_matcher rejit_compile(rejit_parse_result res, rejit_flags flags);
/*! @function rejit_parse_compile
    @brief Parse and compile the given string.
    @discussion
    A wrapper over @link rejit_parse @/link and @link rejit_compile @/link. See
    those two functions for descriptions of the arguments. */
rejit_matcher rejit_parse_compile(const char* str, rejit_parse_error* err,
                                  rejit_flags flags);
/*! @function rejit_match
    @brief Test if @link //apple_ref/doc/functionparam/rejit_match/str @/link
          starts with the pattern in @link
          //apple_ref/doc/functionparam/rejit_match/m @/link.

    @param m The regex to match.
    @param str The string to attempt to match.
    @param groups A pointer to an array of groups. If @link m @/link->@link
                  //apple_ref/doc/structfield/rejit_matcher/groups @/link is 0,
                  then this parameter may be NULL.
    @result The length of the match. */
int rejit_match(rejit_matcher m, const char* str, rejit_group* groups);
/*! @brief Test if @link //apple_ref/doc/functionparam/rejit_match/str @/link
           contains the pattern in @link
           //apple_ref/doc/functionparam/rejit_match/m @/link.

    @discussion
    See @link rejit_match @/link for a description of the rest of the arguments.

    @param tgt If the pattern is found in the string, then this will be set to
               point to that location. Otherwise, it will be NULL. If this
               parameter is NULL, then nothing will occur.
    @result The length of the match. */
int rejit_search(rejit_matcher m, const char* str, const char** tgt,
                 rejit_group* groups);
/*! @function rejit_free_matcher
    @brief Free the given matcher. */
void rejit_free_matcher(rejit_matcher m);

#endif
