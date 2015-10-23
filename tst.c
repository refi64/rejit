#include <libcut.h>
#define REJIT_INSTR
#include "rejit.h"

LIBCUT_TEST(test_chr) {
    rejit_instruction instrs[] = {{ICHR, 'c'}, {ICHR, 'a'}, {INULL}}; // ca
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ca"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cb"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), -1);
}

LIBCUT_TEST(test_dot) {
    rejit_instruction instrs[] = {{IDOT}, {INULL}}; // .
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "\n"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_plus) {
    rejit_instruction instrs[] = {{IPLUS}, {ICHR, 'c'}, {INULL}}; // c+
    instrs[0].value = (intptr_t)&instrs[1];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "cc"), 2);
}

LIBCUT_TEST(test_star) {
    rejit_instruction instrs[] = {{ISTAR}, {ICHR, 'c'}, {INULL}}; // c*
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "cc"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_opt) {
    rejit_instruction instrs[] = {{ICHR, 'a'}, {IOPT}, {ICHR, 'c'}, {IEND},
        {INULL}}; // ac?$
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ac"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "g"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, "acc"), -1);
}

LIBCUT_TEST(test_begin) {
    rejit_instruction instrs[] = {{ISTAR}, {ICHR, 'c'}, {IBEGIN}, {INULL}}; // c*^
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
}

LIBCUT_TEST(test_end) {
    rejit_instruction instrs[] = {{IEND}, {INULL}}; // $
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "c"), -1);
}

LIBCUT_TEST(test_set) {
    rejit_instruction instrs[] = {{ISET, (intptr_t)"xyz"}, {INULL}}; // [xyz]
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "x"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "y"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "z"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_or) {
    rejit_instruction instrs[] = {{IOR}, {ICHR, 'a'}, {ICHR, 'b'}, {INULL}}; // a|b
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_group) {
    rejit_instruction instrs[] = {{IGROUP}, {ICHR, 'a'}, {INULL}}; // (a)
    instrs[0].value = (intptr_t)&instrs[2];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_TEST(test_opt_group) {
    rejit_instruction instrs[] = {{IOPT}, {IGROUP}, {ICHR, 'a'}, {ICHR, 'b'},
        {INULL}}; // (ab)?
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, "b"), 0);
    LIBCUT_TEST_EQ(rejit_match(m, ""), 0);
}

LIBCUT_TEST(test_star_group) {
    rejit_instruction instrs[] = {{ISTAR}, {IGROUP}, {ICHR, 'a'}, {ICHR, 'b'},
        {INULL}}; // (ab)*
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
    rejit_instruction instrs[] = {{IPLUS}, {IGROUP}, {ICHR, 'a'}, {ICHR, 'b'},
        {INULL}}; // (ab)+
    instrs[1].value = (intptr_t)&instrs[4];
    rejit_matcher m = rejit_compile_instrs(instrs, 0);
    LIBCUT_TEST_EQ(rejit_match(m, "ab"), 2);
    LIBCUT_TEST_EQ(rejit_match(m, "abab"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "ababab"), 6);
    LIBCUT_TEST_EQ(rejit_match(m, "ababa"), 4);
    LIBCUT_TEST_EQ(rejit_match(m, "a"), -1);
    LIBCUT_TEST_EQ(rejit_match(m, ""), -1);
}

LIBCUT_MAIN(test_chr, test_dot, test_plus, test_star, test_opt, test_begin,
    test_end, test_set, test_or, test_group, test_opt_group, test_star_group,
    test_plus_group)
