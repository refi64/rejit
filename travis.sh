#!/bin/sh

set -ex

fbuild="fbuild/fbuild-light"

run_base() {
    $fbuild --clean
    $fbuild --cflag=-Ilibcut "$@" || cat build/fbuild.log && build/tst
}

run() {
    run_base "$@"
    run_base --release "$@"
}

run --use-color
run --use-color --cflag=-m32
run --cc=gcc-4.9
run --cc=gcc-4.9 --cflag=-m32
