/* This Source Code Form is subject to the terms of the Mozilla Public
   License, v. 2.0. If a copy of the MPL was not distributed with this
   file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "rejit.h"

rejit_matcher rejit_compile(rejit_parse_result res) {
    return rejit_compile_instrs(res.instrs, res.groups);
}

rejit_matcher rejit_parse_compile(const char* str, rejit_parse_error* err) {
    rejit_matcher m;
    rejit_parse_result p = rejit_parse(str, err);
    if (err->kind != RJ_PE_NONE) return (rejit_matcher)NULL;
    m = rejit_compile(p);
    rejit_free_parse_result(p);
    return m;
}
