#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-or-later
# SPDX-FileCopyrightText: 2026 AnmiTaliDev <anmitalidev@nuros.org>
#
# Regression suite for the compiler: compiles every example through the llvm
# and gcc backends and checks the result against known-good behavior.
set -eu

BUILD_DIR="${1:-build}"
AGNOSTIC="$BUILD_DIR/src/cli/agnostic"
WORK_DIR="$(mktemp -d)"
trap 'rm -rf "$WORK_DIR"' EXIT

fail=0

expect_run() {
    backend=$1
    name=$2
    path=$3
    expected_stdout=$4
    out="$WORK_DIR/${name}_${backend}"

    if ! "$AGNOSTIC" "$path" --backend="$backend" --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name ($backend) did not compile"
        fail=1
        return
    fi

    actual_stdout="$("$out")"
    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo "FAIL: $name ($backend) stdout mismatch"
        echo "  expected: $expected_stdout"
        echo "  actual:   $actual_stdout"
        fail=1
        return
    fi
    echo "PASS: $name ($backend)"
}

expect_llvm_run() { expect_run llvm "$1" "$2" "$3"; }
expect_gcc_run() { expect_run gcc "$1" "$2" "$3"; }

expect_freebsd_compile() {
    name=$1
    path=$2
    out="$WORK_DIR/${name}_freebsd"

    if ! "$AGNOSTIC" "$path" --backend=llvm --target-os=freebsd --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (freebsd) did not compile"
        fail=1
        return
    fi
    echo "PASS: $name (freebsd, compile-only; llvm+gcc execution verified manually on a FreeBSD VM, not in CI)"
}

expect_reject() {
    name=$1
    path=$2
    expected_error=$3
    out="$WORK_DIR/${name}_reject"

    if "$AGNOSTIC" "$path" --backend=llvm --output="$out" >"$out.log" 2>&1; then
        echo "FAIL: $name expected a compile error, but compiled"
        fail=1
        return
    fi
    if ! grep -qF "$expected_error" "$out.log"; then
        echo "FAIL: $name error message mismatch"
        echo "  expected: $expected_error"
        fail=1
        return
    fi
    echo "PASS: $name (reject)"
}

expect_llvm_run closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_llvm_run structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_llvm_run comptime_platform "examples/comptime_platform.agn" "1"
expect_llvm_run hello "examples/hello.agn" "Hello, World!"
expect_llvm_run fizzbuzz "examples/fizzbuzz.agn" \
    "$(printf '1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz')"
expect_llvm_run primes "examples/primes.agn" "$(printf '2\n3\n5\n7\n11\n13\n17\n19\n23\n29')"
expect_llvm_run bubble_sort "examples/bubble_sort.agn" "$(printf '1\n2\n3\n4\n5\n7\n8\n9')"
expect_llvm_run strings_demo "examples/strings_demo.agn" "$(printf 'Hello, Agnostic!\n16\n-1\n1\n0\ntrue\nfalse')"
expect_llvm_run fibonacci "examples/fibonacci.agn" \
    "$(printf '0\n1\n1\n2\n3\n5\n8\n13\n21\n34\n55\n89\n144\n233\n377')"
expect_llvm_run generics "examples/generics.agn" "$(printf 'true\n5\nfalse\ndivide by zero\ntrue\n3\nfalse')"
expect_llvm_run floats "examples/floats.agn" \
    "$(printf '12.566360\n4.000000\n-1.000000\n3.750000\n0.600000\n-1.500000\nx < y\n2.000000\n5.500000')"
expect_llvm_run comptime_eval "examples/comptime_eval.agn" "$(printf '55\n10\ncombined true')"
expect_llvm_run comptime_generics "examples/comptime_generics.agn" "$(printf '7\n10\n42\n5\n6')"
expect_llvm_run comptime_generic_values "examples/comptime_generic_values.agn" "$(printf '15\n15\n42\n21')"
expect_llvm_run casts "examples/casts.agn" "$(printf '44\n255\n-56\n3\n1\n42\nhi')"

expect_gcc_run closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_gcc_run structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_gcc_run comptime_platform "examples/comptime_platform.agn" "1"
expect_gcc_run hello "examples/hello.agn" "Hello, World!"
expect_gcc_run fizzbuzz "examples/fizzbuzz.agn" \
    "$(printf '1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz')"
expect_gcc_run primes "examples/primes.agn" "$(printf '2\n3\n5\n7\n11\n13\n17\n19\n23\n29')"
expect_gcc_run bubble_sort "examples/bubble_sort.agn" "$(printf '1\n2\n3\n4\n5\n7\n8\n9')"
expect_gcc_run strings_demo "examples/strings_demo.agn" "$(printf 'Hello, Agnostic!\n16\n-1\n1\n0\ntrue\nfalse')"
expect_gcc_run fibonacci "examples/fibonacci.agn" \
    "$(printf '0\n1\n1\n2\n3\n5\n8\n13\n21\n34\n55\n89\n144\n233\n377')"
expect_gcc_run generics "examples/generics.agn" "$(printf 'true\n5\nfalse\ndivide by zero\ntrue\n3\nfalse')"
expect_gcc_run floats "examples/floats.agn" \
    "$(printf '12.566360\n4.000000\n-1.000000\n3.750000\n0.600000\n-1.500000\nx < y\n2.000000\n5.500000')"
expect_gcc_run comptime_eval "examples/comptime_eval.agn" "$(printf '55\n10\ncombined true')"
expect_gcc_run comptime_generics "examples/comptime_generics.agn" "$(printf '7\n10\n42\n5\n6')"
expect_gcc_run comptime_generic_values "examples/comptime_generic_values.agn" "$(printf '15\n15\n42\n21')"
expect_gcc_run casts "examples/casts.agn" "$(printf '44\n255\n-56\n3\n1\n42\nhi')"

expect_llvm_run math_stdlib "scripts/testdata/math_test.agn" \
    "$(printf '7\n3\n1024\n9\n6\n12\n120\ntrue\nfalse\n10\n55\ntrue\nfalse\n55')"
expect_llvm_run strings_runtime "scripts/testdata/strings_runtime_test.agn" \
    "$(printf 'foobar\nhello Agnostic, value=00042')"
expect_llvm_run string_stdlib "scripts/testdata/string_test.agn" \
    "$(printf '5\n0\n-1\n1\nfoobar\ntrue\nfalse')"

expect_gcc_run math_stdlib "scripts/testdata/math_test.agn" \
    "$(printf '7\n3\n1024\n9\n6\n12\n120\ntrue\nfalse\n10\n55\ntrue\nfalse\n55')"
expect_gcc_run strings_runtime "scripts/testdata/strings_runtime_test.agn" \
    "$(printf 'foobar\nhello Agnostic, value=00042')"
expect_gcc_run string_stdlib "scripts/testdata/string_test.agn" \
    "$(printf '5\n0\n-1\n1\nfoobar\ntrue\nfalse')"

expect_llvm_run os_string "scripts/testdata/os_string_test.agn" \
    "$(printf 'HELLO\nworld\ntrue\n6\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\n101\nworld\nargcount_ok\ntrue\n9')"
expect_gcc_run os_string "scripts/testdata/os_string_test.agn" \
    "$(printf 'HELLO\nworld\ntrue\n6\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\n101\nworld\nargcount_ok\ntrue\n9')"

expect_freebsd_compile closures "examples/closures.agn"
expect_freebsd_compile structs "examples/structs.agn"
expect_freebsd_compile comptime_platform "examples/comptime_platform.agn"
expect_freebsd_compile hello "examples/hello.agn"
expect_freebsd_compile fizzbuzz "examples/fizzbuzz.agn"
expect_freebsd_compile primes "examples/primes.agn"
expect_freebsd_compile bubble_sort "examples/bubble_sort.agn"
expect_freebsd_compile strings_demo "examples/strings_demo.agn"
expect_freebsd_compile fibonacci "examples/fibonacci.agn"
expect_freebsd_compile generics "examples/generics.agn"
expect_freebsd_compile floats "examples/floats.agn"
expect_freebsd_compile comptime_eval "examples/comptime_eval.agn"
expect_freebsd_compile comptime_generics "examples/comptime_generics.agn"
expect_freebsd_compile comptime_generic_values "examples/comptime_generic_values.agn"
expect_freebsd_compile casts "examples/casts.agn"
expect_freebsd_compile math_stdlib "scripts/testdata/math_test.agn"
expect_freebsd_compile strings_runtime "scripts/testdata/strings_runtime_test.agn"
expect_freebsd_compile string_stdlib "scripts/testdata/string_test.agn"
expect_freebsd_compile os_string "scripts/testdata/os_string_test.agn"

expect_reject bool_to_int "scripts/testdata/reject/bool_to_int.agn" "declared as i64, initialized with bool"
expect_reject int_to_bool "scripts/testdata/reject/int_to_bool.agn" "declared as bool, initialized with i64"
expect_reject ptr_to_int "scripts/testdata/reject/ptr_to_int.agn" "declared as i64, initialized with *i64"
expect_reject int_to_ptr "scripts/testdata/reject/int_to_ptr.agn" "declared as *i64, initialized with i64"
expect_reject ptr_store "scripts/testdata/reject/ptr_store.agn" "pointer assignment: expected i64, got bool"
expect_reject int_condition "scripts/testdata/reject/int_condition.agn" "condition must be bool, got i64"
expect_reject bool_int_compare "scripts/testdata/reject/bool_int_compare.agn" "cannot compare bool with i64"
expect_reject cast_to_bool "scripts/testdata/reject/cast_to_bool.agn" "cannot cast i64 to bool"
expect_reject ptr_cast_width "scripts/testdata/reject/ptr_cast_width.agn" "cannot cast *i64 to i32"
expect_reject unknown_var_type "scripts/testdata/reject/unknown_var_type.agn" "unknown type 'i46' for variable 'x'"
expect_reject unknown_param_type "scripts/testdata/reject/unknown_param_type.agn" "unknown type 'strng' for parameter 'a' (did you mean 'string'?)"
expect_reject unknown_return_type "scripts/testdata/reject/unknown_return_type.agn" "unknown type 'boool' for the return value"
expect_reject unknown_field_type "scripts/testdata/reject/unknown_field_type.agn" "unknown type 'i46' for field 'x' of struct 'Point'"
expect_reject unknown_array_type "scripts/testdata/reject/unknown_array_type.agn" "unknown type 'u9' for the elements of array 'arr'"
expect_reject unknown_pointee_type "scripts/testdata/reject/unknown_pointee_type.agn" "unknown type 'flot' for variable 'p'"

exit $fail
