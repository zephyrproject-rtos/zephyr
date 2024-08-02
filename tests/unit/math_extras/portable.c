/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test the portable version of the math_extras.h functions */
#define PORTABLE_MISC_MATH_EXTRAS 1
#define VNAME(N) test_portable_##N
#include "tests.inc"

ZTEST(math_extras_portable, test_portable_u32_add) {
	run_portable_u32_add();
}

ZTEST(math_extras_portable, test_portable_u32_mul) {
	run_portable_u32_mul();
}

ZTEST(math_extras_portable, test_portable_u64_add) {
	run_portable_u64_add();
}

ZTEST(math_extras_portable, test_portable_u64_mul) {
	run_portable_u64_mul();
}

ZTEST(math_extras_portable, test_portable_size_add) {
	run_portable_size_add();
}

ZTEST(math_extras_portable, test_portable_size_mul) {
	run_portable_size_mul();
}

ZTEST(math_extras_portable, test_portable_u32_clz) {
	run_portable_u32_clz();
}

ZTEST(math_extras_portable, test_portable_u64_clz) {
	run_portable_u64_clz();
}

ZTEST(math_extras_portable, test_portable_u32_ctz) {
	run_portable_u32_ctz();
}

ZTEST(math_extras_portable, test_portable_u64_ctz) {
	run_portable_u64_ctz();
}

ZTEST_SUITE(math_extras_portable, NULL, NULL, NULL, NULL, NULL);
