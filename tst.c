#include <libcut.h>
#include "rejit.h"

LIBCUT_TEST(test_tokenize) {
    rejit_parse_error err;
    err.kind = RJ_PE_NONE;
    err.pos = 0;
    const char s[] = "A(bC)*+?d\\+e";
    rejit_token_list tokens = rejit_tokenize(s, &err);
    LIBCUT_TEST_EQ(err.kind, RJ_PE_NONE);

    LIBCUT_TEST_EQ(tokens.len, 8);

    LIBCUT_TEST_EQ(tokens.tokens[0].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[0].pos, (char*)s);
    LIBCUT_TEST_EQ(tokens.tokens[0].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[1].kind, RJ_TLP);
    LIBCUT_TEST_EQ(tokens.tokens[1].pos, s+1);
    LIBCUT_TEST_EQ(tokens.tokens[1].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[2].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[2].pos, s+2);
    LIBCUT_TEST_EQ(tokens.tokens[2].len, 2);

    LIBCUT_TEST_EQ(tokens.tokens[3].kind, RJ_TRP);
    LIBCUT_TEST_EQ(tokens.tokens[3].pos, s+4);
    LIBCUT_TEST_EQ(tokens.tokens[3].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[4].kind, RJ_TSTAR);
    LIBCUT_TEST_EQ(tokens.tokens[4].pos, s+5);
    LIBCUT_TEST_EQ(tokens.tokens[4].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[5].kind, RJ_TPLUS);
    LIBCUT_TEST_EQ(tokens.tokens[5].pos, s+6);
    LIBCUT_TEST_EQ(tokens.tokens[5].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[6].kind, RJ_TQ);
    LIBCUT_TEST_EQ(tokens.tokens[6].pos, s+7);
    LIBCUT_TEST_EQ(tokens.tokens[6].len, 1);

    LIBCUT_TEST_EQ(tokens.tokens[7].kind, RJ_TWORD);
    LIBCUT_TEST_EQ(tokens.tokens[7].pos, s+8);
    LIBCUT_TEST_EQ(tokens.tokens[7].len, 4);
}

LIBCUT_TEST(test_chr) {
    rejit_instruction instrs[] = {{RJ_ICHR, 'c'}, {RJ_ICHR, 'a'}, {RJ_INULL}}; // ca
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ca"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cb"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), -1);
}

LIBCUT_TEST(test_dot) {
    rejit_instruction instrs[] = {{RJ_IDOT}, {RJ_INULL}}; // .
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "\n"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_plus) {
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_ICHR, 'c'}, {RJ_INULL}}; // c+
    instrs[0].value = (intptr_t)&instrs[1];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cc"), 2);
}

LIBCUT_TEST(test_star) {
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_ICHR, 'c'}, {RJ_INULL}}; // c*
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "cc"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_opt) {
    rejit_instruction instrs[] = {{RJ_ICHR, 'a'}, {RJ_IOPT}, {RJ_ICHR, 'c'}, {RJ_IEND},
        {RJ_INULL}}; // ac?$
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ac"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "g"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "acc"), -1);
}

LIBCUT_TEST(test_begin) {
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_ICHR, 'c'}, {RJ_IBEGIN}, {RJ_INULL}}; // c*^
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
}

LIBCUT_TEST(test_end) {
    rejit_instruction instrs[] = {{RJ_IEND}, {RJ_INULL}}; // $
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
}

LIBCUT_TEST(test_set) {
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)"xyz"}, {RJ_INULL}}; // [xyz]
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "x"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "y"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "z"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_or) {
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_ICHR, 'a'}, {RJ_ICHR, 'b'}, {RJ_INULL}};
        // a|b
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_group) {
    rejit_instruction instrs[] = {{RJ_IGROUP}, {RJ_ICHR, 'a'}, {RJ_INULL}}; // (a)
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_opt_group) {
    rejit_instruction instrs[] = {{RJ_IOPT}, {RJ_IGROUP}, {RJ_ICHR, 'a'}, {RJ_ICHR, 'b'},
        {RJ_INULL}}; // (ab)?
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_star_group) {
    rejit_instruction instrs[] = {{RJ_ISTAR}, {RJ_IGROUP}, {RJ_ICHR, 'a'}, {RJ_ICHR, 'b'},
        {RJ_INULL}}; // (ab)*
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "abab"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "ababab"), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "ababa"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_plus_group) {
    rejit_instruction instrs[] = {{RJ_IPLUS}, {RJ_IGROUP}, {RJ_ICHR, 'a'}, {RJ_ICHR, 'b'},
        {RJ_INULL}}; // (ab)+
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "abab"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "ababab"), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "ababa"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_mplus) {
    rejit_instruction instrs[] = {{RJ_IMPLUS}, {RJ_IDOT}, {RJ_ICHR, 'b'}, {RJ_INULL}}; // .+?b
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "abcb"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "abb"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bb"), 2);
}

LIBCUT_TEST(test_mstar) {
    rejit_instruction instrs[] = {{RJ_IMSTAR}, {RJ_IDOT}, {RJ_ICHR, 'b'}, {RJ_INULL}}; // .*?b
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "abcb"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "abb"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bb"), 1);
}

LIBCUT_TEST(test_or_mixed) {
    rejit_instruction instrs[] = {{RJ_IOR}, {RJ_ICHR, 'b'}, {RJ_ISTAR}, {RJ_ICHR, 'a'},
        {RJ_INULL}}; // (b|a*)
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "aaaa"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_search) {
    rejit_instruction instrs[] = {{RJ_ICHR, 'a'}, {RJ_INULL}}; // a
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    const char* tgt;
    LIBCUT_TEST_EQ(rejit_search(m, "abc", &tgt), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    LIBCUT_TEST_EQ(rejit_search(m, "babc", &tgt), 1);
    LIBCUT_TEST_EQ(*tgt, 'c');
    tgt = NULL;
    LIBCUT_TEST_EQ(rejit_search(m, "b", &tgt), -1);
    LIBCUT_TEST_EQ((void*)tgt, NULL);
}

LIBCUT_TEST(test_set_and_dot) {
    rejit_instruction instrs[] = {{RJ_ISET, (intptr_t)"abc"}, {RJ_IDOT}, {RJ_INULL}};
        // [abc].
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ad"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "bd"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "cd"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "c\n"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "\n"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_MAIN(test_tokenize, test_chr, test_dot, test_plus, test_star, test_opt, test_begin,
    test_end, test_set, test_or, test_group, test_opt_group, test_star_group,
    test_plus_group, test_mplus, test_mstar, test_or_mixed, test_set_and_dot,

    test_search)
