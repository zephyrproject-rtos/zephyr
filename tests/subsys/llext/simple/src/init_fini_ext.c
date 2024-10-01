/*
 * Copyright (c) 2024 Arduino SA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * This test checks for proper support of ELF init arrays. This processing is
 * performed by llext_bootstrap(), which gets the array of function pointers
 * from LLEXT via the llext_get_fn_table() syscall.
 *
 * Each function in this test shifts the number left by 4 bits and sets the
 * lower 4 bits to a specific value. The proper init sequence (preinit_fn_1,
 * preinit_fn_2, init_fn) would leave number set to 0x123; the termination
 * function will further shift the number to 0x1234. If a different result is
 * detected, then either not all routines were executed, or their order was not
 * correct.
 */

#include <stdint.h>
#include <zephyr/llext/symbol.h>
#include <zephyr/toolchain.h>
#include <zephyr/sys/printk.h>
#include <zephyr/ztest_assert.h>

static int number;
EXPORT_SYMBOL(number);

static void preinit_fn_1(void)
{
	number = 1;
}

static void preinit_fn_2(void)
{
	number <<= 4;
	number |= 2;
}

static void init_fn(void)
{
	number <<= 4;
	number |= 3;
}

static void fini_fn(void)
{
	number <<= 4;
	number |= 4;
}

static const void *const preinit_fn_ptrs[] __used Z_GENERIC_SECTION(".preinit_array") = {
	preinit_fn_1,
	preinit_fn_2
};
static const void *const init_fn_ptrs[] __used Z_GENERIC_SECTION(".init_array") = {
	init_fn
};
static const void *const fini_fn_ptrs[] __used Z_GENERIC_SECTION(".fini_array") = {
	fini_fn
};

void test_entry(void)
{
	/* fini_fn() is not called yet, so we expect 0x123 here */
	const int expected = (((1 << 4) | 2) << 4) | 3;

	zassert_equal(number, expected, "got 0x%x instead of 0x%x during test",
		      number, expected);
}
EXPORT_SYMBOL(test_entry);
