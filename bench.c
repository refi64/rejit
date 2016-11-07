/* Any copyright is dedicated to the Public Domain.
   http://creativecommons.org/publicdomain/zero/1.0/ */

#include <sys/time.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <rejit.h>

#define ITERATIONS 1000000

double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + (ts.tv_nsec / 1e9);
}

int bench(const char* regex, const char* s, int it, rejit_flags flags) {
    int i;
    double start, end, diff;
    int r;
    rejit_parse_error err;
    rejit_group* groups = NULL;
    rejit_matcher m = rejit_parse_compile(regex, &err, RJ_FNONE);
    if (err.kind != RJ_PE_NONE) {
        fprintf(stderr, "Error compiling regex (pos: %zu)\n", err.pos);
        return 1;
    }

    if (m->groups) groups = calloc(m->groups, sizeof(rejit_group));
    start = get_time();
    r = rejit_match(m, s, groups);
    for (i=0; i<it; ++i) rejit_match(m, s, groups);
    end = get_time();
    diff = end-start;

    printf("Time spent: %.2fs (average %fms per match, matched: %s)\n", diff,
           (diff*1000)/it, r == -1 ? "false" : "true");
    free(groups);
    rejit_free_matcher(m);
    return 0;
}

void usage(const char* prog) {
    fprintf(stderr, "usage: %s <regex to benchmark> <sample string> "
                    "[iterations: -i%d] [unicode: -u]\n", prog, ITERATIONS);
}

int main(int argc, char** argv) {
    int i, it = ITERATIONS;
    rejit_flags flags = RJ_FNONE;

    if (argc < 3) {
        usage(argv[0]);
        return 1;
    }

    for (i=3; i<argc; ++i) {
        const char* arg = argv[i];

        if (*arg != '-') {
            usage(argv[0]);
            return 1;
        }
        arg++;
        if (!*arg) {
            fprintf(stderr, "empty argument\n");
            return 1;
        }

        for (; *arg; ++arg) {
            switch (*arg) {
            case 'u':
                flags |= RJ_FUNICODE;
                break;
            case 'i':
                it = atoi(arg+1);
                if (it <= 0) {
                    fprintf(stderr, "%s is not a valid # of iterations\n",
                            arg+1);
                    return 1;
                }
                goto out;
            default:
                fprintf(stderr, "invalid argument: %s\n", arg);
                return 1;
            }
        }

        out:;
    }

    return bench(argv[1], argv[2], it, flags);
}
