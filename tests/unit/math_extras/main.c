/*
 * Copyright (c) 2019 Facebook
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/* Test the normal version of the math_extras.h functions */
#define VNAME(N) run_##N
#include "tests.inc"

/* clang-format off */
ZTEST(math_extras, test_u32_add) {
	run_u32_add();
}

ZTEST(math_extras, test_u32_mul) {
	run_u32_mul();
}

ZTEST(math_extras, test_u64_add) {
	run_u64_add();
}

ZTEST(math_extras, test_u64_mul) {
	run_u64_mul();
}

ZTEST(math_extras, test_size_add) {
	run_size_add();
}

ZTEST(math_extras, test_size_mul) {
	run_size_mul();
}

ZTEST(math_extras, test_u32_clz) {
	run_u32_clz();
}

ZTEST(math_extras, test_u64_clz) {
	run_u64_clz();
}

ZTEST(math_extras, test_u32_ctz) {
	run_u32_ctz();
}

ZTEST(math_extras, test_u64_ctz) {
	run_u64_ctz();
}

/* clang-format on */

ZTEST_SUITE(math_extras, NULL, NULL, NULL, NULL, NULL);
