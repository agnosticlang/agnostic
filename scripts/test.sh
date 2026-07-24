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
    name=$1
    path=$2
    expected_stdout=$3
    out="$WORK_DIR/${name}_llvm"

    if ! "$AGNOSTIC" "$path" --backend=llvm --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (llvm) did not compile"
        fail=1
        return
    fi

    actual_stdout="$("$out")"
    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo "FAIL: $name (llvm) stdout mismatch"
        echo "  expected: $expected_stdout"
        echo "  actual:   $actual_stdout"
        fail=1
        return
    fi
    echo "PASS: $name (llvm)"
}

expect_nvm_compile() {
    name=$1
    path=$2
    should_succeed=$3
    out="$WORK_DIR/${name}_nvm"

    if "$AGNOSTIC" "$path" --backend=nvm --output="$out" >/dev/null 2>&1; then
        rc=0
    else
        rc=1
    fi

    if [ "$should_succeed" = "yes" ] && [ "$rc" -ne 0 ]; then
        echo "FAIL: $name (nvm) expected to compile, but errored"
        fail=1
        return
    fi
    if [ "$should_succeed" = "no" ] && [ "$rc" -eq 0 ]; then
        echo "FAIL: $name (nvm) expected to be rejected, but compiled"
        fail=1
        return
    fi
    echo "PASS: $name (nvm)"
}

expect_llvm_run closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_llvm_run structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_llvm_run comptime_platform "examples/comptime_platform.agn" "1"
expect_llvm_run inlineasm "examples/inlineasm.agn" ""

expect_nvm_compile closures "examples/closures.agn" no            # function values/closures unsupported
expect_nvm_compile structs "examples/structs.agn" yes
expect_nvm_compile comptime_platform "examples/comptime_platform.agn" no   # function-valued struct field unsupported
expect_nvm_compile inlineasm "examples/inlineasm.agn" yes

expect_llvm_run math_stdlib "scripts/testdata/math_test.agn" \
    "$(printf '7\n3\n1024\n9\n6\n12\n120\n1\n0\n10\n55\n1\n0\n55')"
expect_llvm_run string_stdlib "scripts/testdata/string_test.agn" \
    "$(printf '5\n0\n-1\n1\nfoobar\n1\n0')"

expect_nvm_compile math_stdlib "scripts/testdata/math_test.agn" yes
expect_nvm_compile string_stdlib "scripts/testdata/string_test.agn" yes

exit $fail
