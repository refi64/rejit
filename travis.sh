#!/bin/sh

set -ex

fbuild="fbuild/fbuild-light"

run() {
    $fbuild --clean
    $fbuild --cflag=-Ilibcut "$@" || cat build/fbuild.log && build/tst
}

run --use-color
run --use-color --cflag=-m32
run --cc=gcc
run --cc=gcc --cflag=-m32
