#!/bin/sh
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#
# Regression suite for the compiler: compiles every example through both the
# llvm and nvm backends and checks the result against known-good behavior.
set -eu

BUILD_DIR="${1:-build}"
AGNOSTIC="$BUILD_DIR/src/cli/agnostic"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

fail=0

expect_llvm_run() {
    example=$1
    expected_stdout=$2
    out="$WORK_DIR/${example}_llvm"

    if ! "$AGNOSTIC" "examples/${example}.agn" --backend=llvm --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $example (llvm) did not compile"
        fail=1
        return
    fi

    actual_stdout="$("$out")"
    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo "FAIL: $example (llvm) stdout mismatch"
        echo "  expected: $expected_stdout"
        echo "  actual:   $actual_stdout"
        fail=1
        return
    fi
    echo "PASS: $example (llvm)"
}

expect_nvm_compile() {
    example=$1
    should_succeed=$2
    out="$WORK_DIR/${example}_nvm"

    if "$AGNOSTIC" "examples/${example}.agn" --backend=nvm --output="$out" >/dev/null 2>&1; then
        rc=0
    else
        rc=1
    fi

    if [ "$should_succeed" = "yes" ] && [ "$rc" -ne 0 ]; then
        echo "FAIL: $example (nvm) expected to compile, but errored"
        fail=1
        return
    fi
    if [ "$should_succeed" = "no" ] && [ "$rc" -eq 0 ]; then
        echo "FAIL: $example (nvm) expected to be rejected, but compiled"
        fail=1
        return
    fi
    echo "PASS: $example (nvm)"
}

expect_llvm_run closures "$(printf '1\n2\n3\n42')"
expect_llvm_run structs "$(printf '25\n4\n5\n10')"
expect_llvm_run comptime_platform "1"
expect_llvm_run inlineasm ""

expect_nvm_compile closures no            # function values/closures unsupported
expect_nvm_compile structs yes
expect_nvm_compile comptime_platform no   # function-valued struct field unsupported
expect_nvm_compile inlineasm yes

exit $fail
