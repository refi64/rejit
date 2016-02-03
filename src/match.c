#define REJIT_INSTR
#include "rejit.h"

#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>
#include "dynasm/dasm_proto.h"

#if !defined(MAP_ANONYMOUS) && defined(MAP_ANON)
#define MAP_ANONYMOUS MAP_ANON
#endif

#define MAXSZ 100

static void compile_one(dasm_State**, rejit_instruction*, int*, int*);

#define GROW dasm_growpc(Dst, ++*pcl)

static unsigned long genmagic(const char* s, char* min) {
    unsigned long res=0;
    const char* b=s;
    // Get minimum.
    *min = *s;
    for (b=s; *s; ++s) if (*s < *min) *min = *s;
    s=b;
    while (*s) res |= 1<<(*s++-*min);
    return res;
}

static void unskip(rejit_instruction* instr) {
    rejit_instruction* i;
    if (instr->kind > RJ_ISKIP) instr->kind -= RJ_ISKIP;
    if (instr->kind == RJ_IGROUP)
        for (i = (rejit_instruction*)instr++->value; instr != i; ++instr)
            unskip(instr);
    else {
        if (instr->kind > RJ_IVARG) unskip((rejit_instruction*)instr->value);
        if (instr->kind > RJ_IARG) unskip(instr+1);
    }
}

static void skip(rejit_instruction* instr) {
    if (instr->kind < RJ_ISKIP) instr->kind += RJ_ISKIP;
}

#include "codegen.c"

static void* link_and_encode(dasm_State** d) {
    size_t sz;
    void* buf;
    #ifdef DEBUG
    FILE* f;
    #endif
    dasm_link(d, &sz);
    buf = mmap(0, sz, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    dasm_encode(d, buf);
    #ifdef DEBUG
    f = fopen("/tmp/.rejit.dis", "w");
    if (f) {
        fwrite(buf, 1, sz, f);
        fclose(f);
    }
    #endif
    mprotect(buf, sz, PROT_READ | PROT_EXEC);
    return buf;
}

static rejit_func compile(dasm_State** d, rejit_instruction* instrs) {
    int i;
    void* labels[lbl__MAX];
    dasm_setupglobal(d, labels, lbl__MAX);
    dasm_setup(d, actions);

    int errpc=0, pcl=1;
    dasm_growpc(d, 1);

    compile_prolog(d);
    for (i=0; instrs[i].kind; ++i) compile_one(d, &instrs[i], &errpc, &pcl);
    compile_epilog(d);

    return link_and_encode(d);
}

rejit_matcher rejit_compile_instrs(rejit_instruction* instrs, int groups) {
    rejit_func func;
    rejit_matcher res;
    dasm_State* d;
    dasm_init(&d, DASM_MAXSECTION);
    func = compile(&d, instrs);
    dasm_free(&d);
    res = malloc(sizeof(struct rejit_matcher_type));
    if (!res) return NULL;
    res->func = func;
    res->groups = groups;
    return res;
}

int rejit_match(rejit_matcher m, const char* str) { return m->func(str); }
int rejit_search(rejit_matcher m, const char* str, const char** tgt) {
    int res = -1;
    for (;res == -1 && *str; ++str) res = rejit_match(m, str);
    if (tgt != NULL && res != -1) *tgt = str+1;
    return res;
}

void rejit_free_matcher(rejit_matcher m) { munmap(m->func, m->sz); free(m); }
