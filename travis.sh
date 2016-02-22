#!/bin/sh

set -ex

fbuild="fbuild/fbuild-light"

run() {
    $fbuild --clean
    $fbuild --use-color --cflag=-Ilibcut "$@" || cat build/fbuild.log && build/tst
}

run
run --cflag=-m32
run --cc=gcc
run --cc=gcc --cflag=-m32
