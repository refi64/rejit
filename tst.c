/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include <libcut.h>
#include "rejit.h"


Rune* str2rune(__libcut_ctx_t* ctx, char* str) {
    Rune* buf = malloc(256), *ptr = buf;
    LIBCUT_TRACK(buf);
    size_t len=strlen(str);

    while (*str) str += chartorune(ptr++, str);
    *ptr = 0;
    return buf;
}


char* rune2str(__libcut_ctx_t* ctx, Rune* rstr) {
    char* buf = malloc(256), *ptr = buf;
    LIBCUT_TRACK(buf);
    size_t len = runestrlen(rstr);

    while (*rstr) runetochar(ptr++, rstr++);
    *ptr = 0;
    return buf;
}


#define R(s) str2rune(ctx, s)

#define TEST_RUNEEQ(a, b) LIBCUT_TEST_CMP(rune2str(a), rune2str(b), \
                                          runestrcmp(a, b) == 0, "!=")
#define TEST_RUNEEQS(a, bs) TEST_RUNEEQ(a, R(bs))


LIBCUT_TEST(test_tokenize) {
    rejit_parse_error err;
    err.kind = RJ_PE_NONE;
    err.pos = 0;
    Rune* r = R("A(bC)*+?[abc]d\\+|e\\2");
    rejit_token_list tokens = rejit_tokenize(r, &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);

    LIBCUT_TEST_EQ(tokens.len, 12);

    LIBCUT_TEST_EQ(tokens.tokens[0].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[0].pos, r);
    LIBCUT_TEST_EQ(tokens.tokens[0].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[1].kind, RJ_TLP);
    LIBCUT_TEST_EQ(tokens.tokens[1].pos, r+1);
    LIBCUT_TEST_EQ(tokens.tokens[1].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[2].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[2].pos, r+2);
    LIBCUT_TEST_EQ(tokens.tokens[2].len, 2);

    LIBCUT_TEST_EQ(tokens.tokens[3].kind, RJ_TRP);
    LIBCUT_TEST_EQ(tokens.tokens[3].pos, r+4);
    LIBCUT_TEST_EQ(tokens.tokens[3].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[4].kind, RJ_TSTAR);
    LIBCUT_TEST_EQ(tokens.tokens[4].pos, r+5);
    LIBCUT_TEST_EQ(tokens.tokens[4].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[5].kind, RJ_TPLUS);
    LIBCUT_TEST_EQ(tokens.tokens[5].pos, r+6);
    LIBCUT_TEST_EQ(tokens.tokens[5].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[6].kind, RJ_TQ);
    LIBCUT_TEST_EQ(tokens.tokens[6].pos, r+7);
    LIBCUT_TEST_EQ(tokens.tokens[6].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[7].kind, RJ_TSET);
    LIBCUT_TEST_EQ(tokens.tokens[7].pos, r+8);
    LIBCUT_TEST_EQ(tokens.tokens[7].len, 5);

    LIBCUT_TEST_EQ(tokens.tokens[8].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[8].pos, r+13);
    LIBCUT_TEST_EQ(tokens.tokens[8].len, 3);

    LIBCUT_TEST_EQ(tokens.tokens[9].kind, RJ_TP);
    LIBCUT_TEST_EQ(tokens.tokens[9].pos, r+16);
    LIBCUT_TEST_EQ(tokens.tokens[9].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[10].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[10].pos, r+17);
    LIBCUT_TEST_EQ(tokens.tokens[10].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[11].kind, RJ_TBACK);
    LIBCUT_TEST_EQ(tokens.tokens[11].pos, r+18);
    LIBCUT_TEST_EQ(tokens.tokens[11].len, 2);
}

#define PARSE(r) res = rejit_parse(r, &err, RJ_FNONE); \
                 LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
#define SPARSE(s) PARSE(R(s))

LIBCUT_TEST(test_parse_word) {
    rejit_parse_error err;
    rejit_parse_result res;
    SPARSE("ab")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_suffix) {
    rejit_parse_error err;
    rejit_parse_result res;
    SPARSE("ab+")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IPLUS);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    PARSE("ab+?")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IMPLUS);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    PARSE("a(?:b?)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "a");

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[1].value, (void*)&res.instrs[4]);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IOPT);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[3].value, "b");

    SPARSE("a{2,5}")

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IREP);
    LIBCUT_TEST_EQ(res.instrs[0].value, 2);
    LIBCUT_TEST_EQ(res.instrs[0].value2, 5);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    SPARSE("a{2}")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IREP);
    LIBCUT_TEST_EQ(res.instrs[0].value, 2);
    LIBCUT_TEST_EQ(res.instrs[0].value2, 2);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS(res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    rejit_parse(R("+"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_SYNTAX);
    LIBCUT_TEST_EQ(err.pos, 0);

    rejit_parse(R("a{a5}"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_INT);
    LIBCUT_TEST_EQ(err.pos, 2);

    rejit_parse(R("a{2a,5}"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_INT);
    LIBCUT_TEST_EQ(err.pos, 3);

    rejit_parse(R("a{2,b}"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_INT);
    LIBCUT_TEST_EQ(err.pos, 4);
}

LIBCUT_TEST(test_parse_group) {
    rejit_parse_error err;
    rejit_parse_result res;
    SPARSE("(ab?)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ICGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IOPT);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[2].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    SPARSE("(?:ab?)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IOPT);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[2].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    rejit_parse(R("((a)"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 4);

    rejit_parse(R("(a))b"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 3);
}

LIBCUT_TEST(test_parse_set) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("[abc]")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ISET);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "abc");
    TEST_RUNEEQS((Rune*)res.instrs[0].value+4, "   ");

    SPARSE("[^abc]")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_INSET);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "abc");
    TEST_RUNEEQS((Rune*)res.instrs[0].value+4, "   ");

    SPARSE("[a-z0-9]")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ISET);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "abcdefghijklmnopqrstuvwxyz0123456789");
    TEST_RUNEEQS((Rune*)res.instrs[0].value+37, "                                    ");

    SPARSE("\\w")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IUSET);
    LIBCUT_TEST_EQ((char)res.instrs[0].value, 'w');
    LIBCUT_TEST_EQ(res.instrs[0].value2, 0);

    SPARSE("\\W")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IUSET);
    LIBCUT_TEST_EQ((char)res.instrs[0].value, 'w');
    LIBCUT_TEST_EQ(res.instrs[0].value2, 1);

    rejit_parse(R("[abc"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_UBOUND);
    LIBCUT_TEST_EQ(err.pos, 0);

    rejit_parse(R("[z-a]"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_RANGE);
    LIBCUT_TEST_EQ(err.pos, 2);
}

LIBCUT_TEST(test_parse_pipe) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("a|b")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IOR);
    LIBCUT_TEST_EQ(res.instrs[0].value, (intptr_t)&res.instrs[2]);
    LIBCUT_TEST_EQ(res.instrs[0].value2, (intptr_t)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    TEST_RUNEEQS((RUne*)res.instrs[2].value, "b");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);

    SPARSE("a|(b|c)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IOR);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value2, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_ICGROUP);
    LIBCUT_TEST_EQ((void*)res.instrs[2].value, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IOR);
    LIBCUT_TEST_EQ((void*)res.instrs[3].value, (void*)&res.instrs[5]);
    LIBCUT_TEST_EQ((void*)res.instrs[3].value2, (void*)&res.instrs[6]);

    LIBCUT_TEST_EQ(res.instrs[4].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[4].value, "b");

    LIBCUT_TEST_EQ(res.instrs[5].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[5].value, "c");

    LIBCUT_TEST_EQ(res.instrs[6].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_lookahead) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("(?=ab)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ILAHEAD);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    SPARSE("(?!ab)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_INLAHEAD);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_lookbehind) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("(?<=ab)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_ILBEHIND);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    SPARSE("(?<!ab)")

    LIBCUT_TEST_EQ(res.maxdepth, 1);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_INLBEHIND);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "ab");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_INULL);

    rejit_parse(R("(?<=a*)"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_LBVAR);
    LIBCUT_TEST_EQ(err.pos, 5);
}

LIBCUT_TEST(test_parse_pipe_suffix) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("|a*")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IOR);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value, (void*)&res.instrs[2]);
    LIBCUT_TEST_EQ((void*)res.instrs[0].value2, (void*)&res.instrs[3]);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_ISTAR);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[2].value, "a");

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_INULL);
}

LIBCUT_TEST(test_parse_other) {
    rejit_parse_error err;
    rejit_parse_result res;

    SPARSE("^a.b$")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IBEGIN);

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[1].value, "a");

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IDOT);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[3].value, "b");

    LIBCUT_TEST_EQ(res.instrs[4].kind, RJ_IEND);

    LIBCUT_TEST_EQ(res.instrs[5].kind, RJ_INULL);

    SPARSE("a\\1\\5b")

    LIBCUT_TEST_EQ(res.maxdepth, 0);

    LIBCUT_TEST_EQ(res.instrs[0].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[0].value, "a");

    LIBCUT_TEST_EQ(res.instrs[1].kind, RJ_IBACK);
    LIBCUT_TEST_EQ(res.instrs[1].value, 0);

    LIBCUT_TEST_EQ(res.instrs[2].kind, RJ_IBACK);
    LIBCUT_TEST_EQ(res.instrs[2].value, 4);

    LIBCUT_TEST_EQ(res.instrs[3].kind, RJ_IWORD);
    TEST_RUNEEQS((Rune*)res.instrs[3].value, "b");

    LIBCUT_TEST_EQ(res.instrs[4].kind, RJ_INULL);
}

LIBCUT_TEST(test_chr) {
    // ca
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)R("ca")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ca"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("cb"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), -1);
}

LIBCUT_TEST(test_dot) {
    // .
    rejit_instruction instrs[] = {{RJ_IDOT}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("\n"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_plus) {
    // c+
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_IWORD, (intptr_t)R("c")},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[1];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("cc"), NULL), 2);
}

LIBCUT_TEST(test_star) {
    // c*
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IWORD, (intptr_t)"c"},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("cc"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
}

LIBCUT_TEST(test_opt) {
    // ac?$
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)"a"}, {RJ_IOPT},
                                  {RJ_IWORD, (intptr_t)"c"}, {RJ_IEND},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ac"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("g"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("acc"), NULL), -1);
}

LIBCUT_TEST(test_rep) {
    // a{2,5}$
    rejit_instruction instrs[] = {{RJ_IREP, 2, 5}, {RJ_IWORD, (intptr_t)"a"},
                                  {RJ_IEND}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("aa"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaa"), NULL), 3);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaa"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaaa"), NULL), 5);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaaaa"), NULL), -1);
}

LIBCUT_TEST(test_begin) {
    // c*^$
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IWORD, (intptr_t)R("c")},
                                  {RJ_IBEGIN}, {RJ_IEND}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), -1);
}

LIBCUT_TEST(test_end) {
    // $
    rejit_instruction instrs[] = {{RJ_IEND}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), -1);
}

LIBCUT_TEST(test_set) {
    // [\txyz]
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)R("\txyz\0    ")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("\t"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("x"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("y"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("z"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_nset) {
    // [^xyz]
    rejit_instruction instrs[] = {{RJ_INSET, (intptr_t)R("\txyz\0    ")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("\t"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("x"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("y"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("z"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
}

LIBCUT_TEST(test_uset) {
    // \w, \d, \W
    rejit_instruction wi[] = {{RJ_IPLUS}, {RJ_IUSET, 'w'}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(wi, 0, 0, RJ_FUNICODE);
    LIBCUT_TEST_EQ(rejit_match(m, R("_"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("Ã"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("1"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("٠"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("!"), NULL), -1);

    rejit_instruction di[] = {{RJ_IPLUS}, {RJ_IUSET, 'd'}, {RJ_INULL}};
    m = rejit_compile_instrs(di, 0, 0, RJ_FUNICODE);
    LIBCUT_TEST_EQ(rejit_match(m, R("1"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("٠"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);

    rejit_instruction nwi[] = {{RJ_IPLUS}, {RJ_IUSET, 'w', 1}, {RJ_INULL}};
    m = rejit_compile_instrs(nwi, 0, 0, RJ_FUNICODE);
    LIBCUT_TEST_EQ(rejit_match(m, R("_"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("Ã"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("1"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("!"), NULL), 1);
}

LIBCUT_TEST(test_or) {
    // a|b
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IWORD, (intptr_t)R("a")},
                                  {RJ_IWORD, (intptr_t)"b"}, {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[0].value2 = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_group) {
    // (a)
    rejit_instruction instrs[] = {{RJ_IGROUP}, {RJ_IWORD, (intptr_t)R("a")},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_cgroup) {
    // (a(b)?)
    rejit_group groups[2];
    Rune* r1 = R("a"), *r2 = R("ab");
    rejit_instruction instrs[] = {{RJ_ICGROUP, 0, 0}, {RJ_IWORD, (intptr_t)R("a")},
                                  {RJ_IOPT}, {RJ_ICGROUP, 0, 1},
                                  {RJ_IWORD, (intptr_t)R("b")}, {RJ_INULL}};
    instrs[0].value = instrs[3].value = (intptr_t)&instrs[5];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 2, RJ_FNONE);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, r1, groups), 1);
    LIBCUT_TEST_EQ(groups[0].begin, r1);
    LIBCUT_TEST_EQ(groups[0].end, r1+1);
    LIBCUT_TEST_EQ(groups[1].begin, NULL);
    LIBCUT_TEST_EQ(groups[1].end, NULL);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, r2, groups), 2);
    LIBCUT_TEST_EQ(groups[0].begin, (Rune*)r2);
    LIBCUT_TEST_EQ(groups[0].end, r2+2);
    LIBCUT_TEST_EQ(groups[1].begin, r2+1);
    LIBCUT_TEST_EQ(groups[1].end, r2+2);

    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, R(""), groups), -1);
    LIBCUT_TEST_EQ(groups[0].begin, NULL);
    LIBCUT_TEST_EQ(groups[0].end, NULL);
    LIBCUT_TEST_EQ(groups[1].begin, NULL);
    LIBCUT_TEST_EQ(groups[1].end, NULL);
}

LIBCUT_TEST(test_opt_group) {
    // (ab)?
    rejit_instruction instrs[] = {{RJ_IOPT}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)R("ab")}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
}

LIBCUT_TEST(test_star_group) {
     // (ab)*
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)R("ab")}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("abab"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("ababab"), NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, R("ababa"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
}

LIBCUT_TEST(test_plus_group) {
    // (ab)+
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)R("ab")}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("abab"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("ababab"), NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, R("ababa"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_lookahead) {
    // (?=ab)a(bc)?
    rejit_instruction instrs[] = {{RJ_ILAHEAD}, {RJ_IWORD, (intptr_t)R("ab")},
                                  {RJ_IWORD, (intptr_t)R("a")}, {RJ_IOPT},
                                  {RJ_IGROUP}, {RJ_IWORD, (intptr_t)R("bc")},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[4].value = (intptr_t)&instrs[6];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abc"), NULL), 3);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_negative_lookahead) {
    // (?!ab)[ab]*
    rejit_instruction instrs[] = {{RJ_INLAHEAD}, {RJ_IWORD, (intptr_t)R("ab")},
                                  {RJ_ISTAR}, {RJ_ISET, (intptr_t)R("ab\0  ")},
                                  {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("bb"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("aa"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), -1);
}

LIBCUT_TEST(test_lookbehind) {
    // a*(?<=a)bc
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IWORD, (intptr_t)R("a")},
                                  {RJ_ILBEHIND}, {RJ_IWORD, (intptr_t)R("a"), 0, 1},
                                  {RJ_IWORD, (intptr_t)R("bc")}, {RJ_INULL}};
    instrs[2].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("aabc"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("abc"), NULL), 3);
    LIBCUT_TEST_EQ(rejit_match(m, R("bc"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_negative_lookbehind) {
    // [ab]*(?<!a)c
    char s[] = "ab\0  ";
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_ISET, (intptr_t)s},
                                  {RJ_INLBEHIND}, {RJ_IWORD, (intptr_t)R("a"), 0, 1},
                                  {RJ_IWORD, (intptr_t)R("c")}, {RJ_INULL}};
    instrs[2].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaabc"), NULL), 5);
    LIBCUT_TEST_EQ(rejit_match(m, R("bc"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("ac"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("bbaabac"), NULL), -1);
}

LIBCUT_TEST(test_mplus) {
    // .+?b
    rejit_instruction instrs[] = {{RJ_IMPLUS}, {RJ_IDOT},
                                  {RJ_IWORD, (intptr_t)R("b")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcb"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abb"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("bb"), NULL), 2);
}

LIBCUT_TEST(test_mstar) {
    // .*?b
    rejit_instruction instrs[] = {{RJ_IMSTAR}, {RJ_IDOT},
                                  {RJ_IWORD, (intptr_t)R("b")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcb"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abb"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("bb"), NULL), 1);
}

LIBCUT_TEST(test_or_mixed) {
    // (b|a*)
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IWORD, (intptr_t)R("b")}, {RJ_ISTAR},
                                  {RJ_IWORD, (intptr_t)R("a")}, {RJ_INULL}};
    instrs[0].value = (intptr_t)&instrs[2];
    instrs[0].value2 = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 2, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaa"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
}

LIBCUT_TEST(test_search) {
    // a
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)R("a")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    const char* tgt;
    LIBCUT_TEST_EQ(rejit_search(m, "abc", &tgt, NULL), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    LIBCUT_TEST_EQ(rejit_search(m, "babc", &tgt, NULL), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    tgt = NULL;
    LIBCUT_TEST_EQ(rejit_search(m, "b", &tgt, NULL), -1);
    LIBCUT_TEST_EQ((void*)tgt, NULL);
}

LIBCUT_TEST(test_match_len) {
    rejit_instruction instrs[3];
    rejit_instruction* ia = &instrs[0], *ib = &instrs[1], *ic = &instrs[2];

    ib->kind = RJ_IWORD;
    ib->value = (intptr_t)R("abc123");
    LIBCUT_TEST_EQ(rejit_match_len(ib), 6);

    ic->kind = RJ_ISET;
    ic->value = (intptr_t)R("abc   ");
    LIBCUT_TEST_EQ(rejit_match_len(ic), 1);

    ia->kind = RJ_IPLUS;
    LIBCUT_TEST_EQ(rejit_match_len(ia), -1);

    ia->kind = RJ_ISTAR;
    LIBCUT_TEST_EQ(rejit_match_len(ia), -1);

    ia->kind = RJ_IREP;
    ia->value = 2;
    ia->value2 = 5;
    LIBCUT_TEST_EQ(rejit_match_len(ia), -1);
    ia->value2 = 2;
    LIBCUT_TEST_EQ(rejit_match_len(ia), 12);

    ia->kind = RJ_IGROUP;
    ia->value = (intptr_t)ic;
    LIBCUT_TEST_EQ(rejit_match_len(ia), 6);
}

LIBCUT_TEST(test_set_and_dot) {
    // [abc].
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)R("abc\0   ")}, {RJ_IDOT},
                                  {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ad"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("bd"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("cd"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("c\n"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("\n"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_or_group) {
    // (ab)|c
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_IGROUP},
                                  {RJ_IWORD, (intptr_t)R("ab")},
                                  {RJ_IWORD, (intptr_t)R("c")}, {RJ_INULL}};
    instrs[0].value = instrs[1].value = (intptr_t)&instrs[3];
    instrs[0].value2 = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 2, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("ab"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("ac"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
}

LIBCUT_TEST(test_back) {
    // (abc)d\1e
    rejit_instruction instrs[] = {{RJ_ICGROUP}, {RJ_IWORD, (intptr_t)R("abc")},
                                  {RJ_IWORD, (intptr_t)R("d")}, {RJ_IBACK, 0},
                                  {RJ_IWORD, (intptr_t)R("e")}, {RJ_INULL}};
    rejit_group group;
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdabce"), &group), 8);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdabe"), &group), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abdabe"), &group), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abdabce"), &group), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdabc"), &group), -1);
}

LIBCUT_TEST(test_dotall) {
    rejit_instruction instrs[] = {{RJ_IDOT}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FDOTALL);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("\n"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_icase_word) {
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)R("aBCd")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FICASE);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("Abcd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("aBcd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("abCd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcD"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("ABCD"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("AbCd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("aBcD"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("ABCd"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcD"), NULL), 4);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcf"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);
}

LIBCUT_TEST(test_icase_set) {
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)R("abcd\0    ")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FICASE);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("A"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("B"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("c"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("C"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("d"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("D"), NULL), 1);
}

LIBCUT_TEST(test_save) {
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)R("a")}, {RJ_ILBEHIND},
                                  {RJ_IBEGIN}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[3];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), -1);
}

LIBCUT_TEST(test_long_word) {
    rejit_instruction instrs[] = {{RJ_IWORD, (intptr_t)R("abcdefghij")}, {RJ_INULL}};
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 0, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdefghij"), NULL), 10);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdefghi"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abcdefghijk"), NULL), 10);
}

LIBCUT_TEST(test_empty_group) {
    // (?:a*)*
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IGROUP}, {RJ_ISTAR},
                                  {RJ_IWORD, (intptr_t)R("a")}, {RJ_INULL}};
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0, 1, RJ_FNONE);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaa"), NULL), 3);
}

LIBCUT_TEST(test_misc) {
    rejit_matcher m;
    rejit_parse_error err;
    rejit_group groups[3];

    m = rejit_parse_compile(R("[Oo]rgani[sz]ation"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("Organization"), NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, R("Organisation"), NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, R("organization"), NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, R("organisation"), NULL), 12);
    LIBCUT_TEST_EQ(rejit_match(m, R("organizatio"), NULL), -1);

    m = rejit_parse_compile(R("hono(?:u)?r(?:able)?"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(rejit_match(m, R("honor"), NULL), 5);
    LIBCUT_TEST_EQ(rejit_match(m, R("honour"), NULL), 6);
    LIBCUT_TEST_EQ(rejit_match(m, R("honorable"), NULL), 9);
    LIBCUT_TEST_EQ(rejit_match(m, R("honourable"), NULL), 10);

    memset(groups, 0, sizeof(groups));
    m = rejit_parse_compile(R("a(b)?c"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("abc"), groups), 3);
    LIBCUT_TEST_STREQ(groups[0].begin, "bc");
    LIBCUT_TEST_STREQ(groups[0].end, "c");
    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, R("ac"), groups), 2);
    LIBCUT_TEST_EQ(groups[0].begin, NULL);
    LIBCUT_TEST_EQ(groups[0].end, NULL);

    m = rejit_parse_compile(R("[ÁÃa]b"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 0);
    LIBCUT_TEST_NE(rejit_match(m, R("Áb"), NULL), -1);
    LIBCUT_TEST_NE(rejit_match(m, R("Ãb"), NULL), -1);
    LIBCUT_TEST_NE(rejit_match(m, R("ab"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("Âb"), NULL), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("b"), NULL), -1);

    m = rejit_parse_compile(R("(?i)a"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("A"), NULL), 1);

    m = rejit_parse_compile(R("(?s)."), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R("\n"), NULL), 1);
    LIBCUT_TEST_EQ(rejit_match(m, R(""), NULL), -1);

    m = rejit_parse_compile(R(".{0,2}(?:a)"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("aa"), NULL), 2);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaa"), NULL), 3);
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaa"), NULL), 3);

    m = rejit_parse_compile(R("aa(?<=a{2})b"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 0);
    LIBCUT_TEST_EQ(rejit_match(m, R("aab"), NULL), 3);

    memset(groups, 0, sizeof(groups));
    m = rejit_parse_compile(R("(a+)(a*)(a+)"), &err, RJ_FNONE);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);
    LIBCUT_TEST_EQ(m->groups, 3);
    LIBCUT_TEST_EQ(rejit_match(m, R("a"), groups), -1);
    LIBCUT_TEST_EQ(rejit_match(m, R("aa"), groups), 2);
    TEST_RUNEEQS(groups[0].begin, "aa");
    TEST_RUNEEQS(groups[0].end, "a");
    TEST_RUNEEQS(groups[1].begin, "a");
    TEST_RUNEEQS(groups[1].end, "a");
    TEST_RUNEEQS(groups[2].begin, "a");
    TEST_RUNEEQS(groups[2].end, "");
    memset(groups, 0, sizeof(groups));
    LIBCUT_TEST_EQ(rejit_match(m, R("aaaaaa"), groups), 6);
    TEST_RUNEEQS(groups[0].begin, "aaaaaa");
    TEST_RUNEEQS(groups[0].end, "a");
    TEST_RUNEEQS(groups[1].begin, "a");
    TEST_RUNEEQS(groups[1].end, "a");
    TEST_RUNEEQS(groups[2].begin, "a");
    TEST_RUNEEQS(groups[2].end, "");
}

LIBCUT_MAIN(
    test_tokenize,

    test_parse_word, test_parse_suffix, test_parse_group, test_parse_set,
    test_parse_pipe, test_parse_lookahead, test_parse_lookbehind,
    test_parse_pipe_suffix, test_parse_other,

    test_chr, test_dot, test_plus, test_star, test_opt, test_rep, test_begin,
    test_end, test_set, test_nset, test_uset, test_or, test_group, test_cgroup,
    test_opt_group, test_star_group, test_plus_group, test_lookahead,
    test_negative_lookahead, test_lookbehind, test_negative_lookbehind,
    test_mplus, test_mstar, test_or_mixed, test_set_and_dot, test_or_group,
    test_back, test_dotall, test_icase_word, test_icase_set, test_save,
    test_long_word, test_empty_group,

    test_search, test_match_len,

    test_misc)
