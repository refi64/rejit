#!/bin/sh

set -ex

fbuild="fbuild/fbuild-light"

run() {
    $fbuild --clean
    $fbuild --cflag=-Ilibcut "$@" || cat build/fbuild.log && build/tst
}

run --use-color
run --use-color --cflag=-m32
run --cc=gcc-4.9
run --cc=gcc-4.9 --cflag=-m32
