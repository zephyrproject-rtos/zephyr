/*
 * Copyright (c) 2021 Friedt Professional Engineering Services, Inc
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>

#include <zephyr.h>
#include <init.h>

#include <ztest.h>

static bool is_all_zero(const void *x, size_t size)
{
	const uint8_t *xx = (const uint8_t *)x;

	for (; size; --size, ++xx) {
		if (*xx != 0) {
			return false;
		}
	}

	return true;
}

struct foo {
	int a;
	void *b;
};

int global_int;
struct foo global_foo;
struct foo global_foo_array[42];

static void test_global(void)
{
	zassert_equal(0, global_int, "global int");
	zassert_true(is_all_zero(&global_foo, sizeof(global_foo)), "global struct");
	zassert_true(is_all_zero(global_foo_array, sizeof(global_foo_array)),
		     "global struct array");
}

static int __noinit static_int;
static struct foo __noinit static_foo;
static struct foo __noinit static_foo_array[42];

static void test_static_file_scope(void)
{
	zassert_equal(0, static_int, "static int");
	zassert_true(is_all_zero(&static_foo, sizeof(static_foo)), "static struct");
	zassert_true(is_all_zero(static_foo_array, sizeof(static_foo_array)),
		     "static struct array");
}

static void test_static_local_scope(void)
{
	static int __noinit local_int;
	static struct foo __noinit local_foo;
	static struct foo __noinit local_foo_array[42];

	zassert_equal(0, local_int, "local int");
	zassert_true(is_all_zero(&local_foo, sizeof(local_foo)), "local struct");
	zassert_true(is_all_zero(local_foo_array, sizeof(local_foo_array)),
		     "local struct array");
}

void test_main(void)
{
	ztest_test_suite(noinit_tests, ztest_unit_test(test_global),
			 ztest_unit_test(test_static_file_scope),
			 ztest_unit_test(test_static_local_scope));

	ztest_run_test_suite(noinit_tests);
}
