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

expect_mem_run() {
    mem=$1
    name=$2
    path=$3
    expected_stdout=$4
    backend=${5:-llvm}
    out="$WORK_DIR/${name}_mem_${mem}_${backend}"

    if ! "$AGNOSTIC" "$path" --backend="$backend" --mem="$mem" --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (mem=$mem, $backend) did not compile"
        fail=1
        return
    fi

    actual_stdout="$(ulimit -v 262144; "$out")"
    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo "FAIL: $name (mem=$mem, $backend) stdout mismatch"
        echo "  expected: $expected_stdout"
        echo "  actual:   $actual_stdout"
        fail=1
        return
    fi
    echo "PASS: $name (mem=$mem, $backend)"
}
expect_gcc_run() { expect_run gcc "$1" "$2" "$3"; }

expect_mem_fatal() {
    mem=$1
    name=$2
    path=$3
    expected_stderr=$4
    expected_code=$5
    out="$WORK_DIR/${name}_fatal_${mem}"

    if ! "$AGNOSTIC" "$path" --mem="$mem" --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (mem=$mem) did not compile"
        fail=1
        return
    fi

    if actual_stderr="$(ulimit -v 262144; "$out" 2>&1 >/dev/null)"; then code=0; else code=$?; fi
    if [ "$code" != "$expected_code" ] || [ "$actual_stderr" != "$expected_stderr" ]; then
        echo "FAIL: $name (mem=$mem) expected exit $expected_code with: $expected_stderr"
        echo "  actual: exit $code with: $actual_stderr"
        fail=1
        return
    fi
    echo "PASS: $name (mem=$mem, fatal)"
}

expect_hurd_compile() {
    name=$1
    path=$2
    out="$WORK_DIR/${name}_hurd"

    if ! "$AGNOSTIC" "$path" --target-os=hurd --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (hurd) did not compile"
        fail=1
        return
    fi
    echo "PASS: $name (hurd, compile-only; not executed)"
}

expect_windows_run() {
    name=$1
    path=$2
    expected_stdout=$3
    out="$WORK_DIR/${name}_windows"

    if ! command -v wine >/dev/null 2>&1 || ! command -v lld-link >/dev/null 2>&1; then
        echo "SKIP: $name (windows; needs wine and lld-link)"
        return
    fi
    if ! "$AGNOSTIC" "$path" --target-os=windows --output="$out" >/dev/null 2>&1; then
        echo "FAIL: $name (windows) did not compile"
        fail=1
        return
    fi

    actual_stdout="$(WINEDEBUG=-all wine "$out.exe" 2>/dev/null)"
    if [ "$actual_stdout" != "$expected_stdout" ]; then
        echo "FAIL: $name (windows) stdout mismatch"
        echo "  expected: $expected_stdout"
        echo "  actual:   $actual_stdout"
        fail=1
        return
    fi
    echo "PASS: $name (windows)"
}

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
expect_llvm_run "odd path \"dq\" 'sq' \$(exit 1)" "examples/hello.agn" "Hello, World!"
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
expect_llvm_run loop_locals "scripts/testdata/loop_locals_test.agn" "1999999"
expect_llvm_run long_strings "scripts/testdata/long_strings_test.agn" \
    "$(printf '3000\n3050\ntrue\n000255|ff|  255|\n-9223372036854775808')"
expect_llvm_run numeric_conversions "scripts/testdata/numeric_conversions_test.agn" \
    "$(printf '%s\n' -100 200 200 -73 -200 203.000000 100.000000 4 8 4 127 true)"
expect_gcc_run os_string "scripts/testdata/os_string_test.agn" \
    "$(printf 'HELLO\nworld\ntrue\n6\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\n101\nworld\nargcount_ok\ntrue\n9')"
expect_gcc_run loop_locals "scripts/testdata/loop_locals_test.agn" "1999999"
expect_gcc_run long_strings "scripts/testdata/long_strings_test.agn" \
    "$(printf '3000\n3050\ntrue\n000255|ff|  255|\n-9223372036854775808')"
expect_gcc_run numeric_conversions "scripts/testdata/numeric_conversions_test.agn" \
    "$(printf '%s\n' -100 200 200 -73 -200 203.000000 100.000000 4 8 4 127 true)"

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

expect_hurd_compile closures "examples/closures.agn"
expect_hurd_compile structs "examples/structs.agn"
expect_hurd_compile comptime_platform "examples/comptime_platform.agn"
expect_hurd_compile hello "examples/hello.agn"
expect_hurd_compile fizzbuzz "examples/fizzbuzz.agn"
expect_hurd_compile primes "examples/primes.agn"
expect_hurd_compile bubble_sort "examples/bubble_sort.agn"
expect_hurd_compile strings_demo "examples/strings_demo.agn"
expect_hurd_compile fibonacci "examples/fibonacci.agn"
expect_hurd_compile generics "examples/generics.agn"
expect_hurd_compile floats "examples/floats.agn"
expect_hurd_compile comptime_eval "examples/comptime_eval.agn"
expect_hurd_compile comptime_generics "examples/comptime_generics.agn"
expect_hurd_compile comptime_generic_values "examples/comptime_generic_values.agn"
expect_hurd_compile casts "examples/casts.agn"
expect_hurd_compile math_stdlib "scripts/testdata/math_test.agn"
expect_hurd_compile strings_runtime "scripts/testdata/strings_runtime_test.agn"
expect_hurd_compile string_stdlib "scripts/testdata/string_test.agn"
expect_hurd_compile os_string "scripts/testdata/os_string_test.agn"

expect_mem_run arc alloc_stress "scripts/testdata/alloc_stress_test.agn" "400000"
expect_mem_run manual alloc_stress "scripts/testdata/alloc_stress_test.agn" "400000"
expect_mem_run manual long_strings "scripts/testdata/long_strings_test.agn" \
    "$(printf '3000\n3050\ntrue\n000255|ff|  255|\n-9223372036854775808')"
expect_mem_run orc long_strings "scripts/testdata/long_strings_test.agn" \
    "$(printf '3000\n3050\ntrue\n000255|ff|  255|\n-9223372036854775808')"
expect_mem_run orc alloc_stress "scripts/testdata/alloc_stress_test.agn" "400000"
expect_mem_fatal arc out_of_memory "scripts/testdata/out_of_memory_test.agn" "fatal error: out of memory" 2
expect_mem_fatal manual out_of_memory "scripts/testdata/out_of_memory_test.agn" "fatal error: out of memory" 2
expect_mem_fatal orc out_of_memory "scripts/testdata/out_of_memory_test.agn" "fatal error: out of memory" 2
expect_mem_run manual closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_mem_run orc closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_mem_run orc closures "examples/closures.agn" "$(printf '1\n2\n3\n42')" gcc
expect_mem_run orc orc_escape "scripts/testdata/orc_escape_test.agn" \
    "$(printf 'hello, orc / hello, orc\nitem-42\nfilled-3\n8\nabc\n42\nseen-21\n25000000')"
expect_mem_run orc orc_escape "scripts/testdata/orc_escape_test.agn" \
    "$(printf 'hello, orc / hello, orc\nitem-42\nfilled-3\n8\nabc\n42\nseen-21\n25000000')" gcc
expect_mem_run manual structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_mem_run orc structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_mem_run manual strings_runtime "scripts/testdata/strings_runtime_test.agn" \
    "$(printf 'foobar\nhello Agnostic, value=00042')"
expect_mem_run orc strings_runtime "scripts/testdata/strings_runtime_test.agn" \
    "$(printf 'foobar\nhello Agnostic, value=00042')"

expect_windows_run closures "examples/closures.agn" "$(printf '1\n2\n3\n42')"
expect_windows_run structs "examples/structs.agn" "$(printf '25\n4\n5\n10')"
expect_windows_run comptime_platform "examples/comptime_platform.agn" "0"
expect_windows_run hello "examples/hello.agn" "Hello, World!"
expect_windows_run "odd path 'sq' \$(exit 1)" "examples/hello.agn" "Hello, World!"
expect_windows_run fizzbuzz "examples/fizzbuzz.agn" \
    "$(printf '1\n2\nFizz\n4\nBuzz\nFizz\n7\n8\nFizz\nBuzz\n11\nFizz\n13\n14\nFizzBuzz\n16\n17\nFizz\n19\nBuzz\nFizz\n22\n23\nFizz\nBuzz\n26\nFizz\n28\n29\nFizzBuzz')"
expect_windows_run primes "examples/primes.agn" "$(printf '2\n3\n5\n7\n11\n13\n17\n19\n23\n29')"
expect_windows_run bubble_sort "examples/bubble_sort.agn" "$(printf '1\n2\n3\n4\n5\n7\n8\n9')"
expect_windows_run strings_demo "examples/strings_demo.agn" "$(printf 'Hello, Agnostic!\n16\n-1\n1\n0\ntrue\nfalse')"
expect_windows_run fibonacci "examples/fibonacci.agn" \
    "$(printf '0\n1\n1\n2\n3\n5\n8\n13\n21\n34\n55\n89\n144\n233\n377')"
expect_windows_run generics "examples/generics.agn" "$(printf 'true\n5\nfalse\ndivide by zero\ntrue\n3\nfalse')"
expect_windows_run floats "examples/floats.agn" \
    "$(printf '12.566360\n4.000000\n-1.000000\n3.750000\n0.600000\n-1.500000\nx < y\n2.000000\n5.500000')"
expect_windows_run comptime_eval "examples/comptime_eval.agn" "$(printf '55\n10\ncombined false')"
expect_windows_run comptime_generics "examples/comptime_generics.agn" "$(printf '7\n10\n42\n5\n6')"
expect_windows_run comptime_generic_values "examples/comptime_generic_values.agn" "$(printf '15\n15\n42\n21')"
expect_windows_run casts "examples/casts.agn" "$(printf '44\n255\n-56\n3\n1\n42\nhi')"
expect_windows_run math_stdlib "scripts/testdata/math_test.agn" \
    "$(printf '7\n3\n1024\n9\n6\n12\n120\ntrue\nfalse\n10\n55\ntrue\nfalse\n55')"
expect_windows_run strings_runtime "scripts/testdata/strings_runtime_test.agn" \
    "$(printf 'foobar\nhello Agnostic, value=00042')"
expect_windows_run string_stdlib "scripts/testdata/string_test.agn" \
    "$(printf '5\n0\n-1\n1\nfoobar\ntrue\nfalse')"
expect_windows_run os_string "scripts/testdata/os_string_test.agn" \
    "$(printf 'HELLO\nworld\ntrue\n6\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\nfalse\ntrue\n101\nworld\nargcount_ok\ntrue\n9')"
expect_windows_run loop_locals "scripts/testdata/loop_locals_test.agn" "1999999"
expect_windows_run long_strings "scripts/testdata/long_strings_test.agn" \
    "$(printf '3000\n3050\ntrue\n000255|ff|  255|\n-9223372036854775808')"
expect_windows_run numeric_conversions "scripts/testdata/numeric_conversions_test.agn" \
    "$(printf '%s\n' -100 200 200 -73 -200 203.000000 100.000000 4 8 4 127 true)"

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
expect_reject narrow_int "scripts/testdata/reject/narrow_int.agn" "declared as i8, initialized with i64 (convert explicitly with 'as i8')"
expect_reject float_to_int "scripts/testdata/reject/float_to_int.agn" "declared as i64, initialized with f64 (convert explicitly with 'as i64')"
expect_reject int64_to_float "scripts/testdata/reject/int64_to_float.agn" "declared as f64, initialized with i64 (convert explicitly with 'as f64')"
expect_reject signed_to_unsigned "scripts/testdata/reject/signed_to_unsigned.agn" "declared as u64, initialized with i8 (convert explicitly with 'as u64')"
expect_reject constant_overflow "scripts/testdata/reject/constant_overflow.agn" "declared as u8, initialized with i64 (constant 256 does not fit in u8)"
expect_reject negative_unsigned "scripts/testdata/reject/negative_unsigned.agn" "(constant -1 does not fit in u32)"
expect_reject operand_constant_overflow "scripts/testdata/reject/operand_constant_overflow.agn" "constant 200 does not fit in i8"
expect_reject mixed_float_int "scripts/testdata/reject/mixed_float_int.agn" "mismatched operand types f64 and i64 (convert one side explicitly with 'as')"
expect_reject mixed_signedness "scripts/testdata/reject/mixed_signedness.agn" "mismatched operand types i32 and u32"
expect_reject narrow_return "scripts/testdata/reject/narrow_return.agn" "return type mismatch: expected u8, got i64 (convert explicitly with 'as u8')"
expect_reject narrow_argument "scripts/testdata/reject/narrow_argument.agn" "argument 0 of 'take': expected i32, got i64 (convert explicitly with 'as i32')"
expect_reject lex_unexpected_char "scripts/testdata/reject/lex_unexpected_char.agn" "lex_unexpected_char.agn:7:19"
expect_reject parse_missing_name "scripts/testdata/reject/parse_missing_name.agn" "expected variable name"
expect_reject generic_type_arg "scripts/testdata/reject/generic_type_arg.agn" "argument 0 of 'identity' must be a type name (in main)"
expect_reject generic_value_arg "scripts/testdata/reject/generic_value_arg.agn" "argument 0 of 'scale' must be a compile-time constant"
expect_reject generic_arity "scripts/testdata/reject/generic_arity.agn" "'Box' expects 1 type argument(s), got 2 (in main)"
expect_reject generic_recursive "scripts/testdata/reject/generic_recursive.agn" "recursive generic instantiation of 'Node' (in Node)"

exit $fail
