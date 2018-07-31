/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <tc_util.h>
#include <linker/linker-defs.h>
#include <ztest.h>

/**
 * @brief Memory protection tests
 * @defgroup kernel_memprotect_tests Memory Protection Tests
 * @ingroup all_tests
 * @{
 * @}
 */
struct test_struct {
	int foo;
	int bar;
	char *baz;
};

/* Check that the __kenrnel* macros work properly */
struct test_struct __kernel kernel_data = {1, 2, NULL};
struct test_struct __kernel_bss kernel_bss;
struct test_struct __kernel_noinit kernel_noinit;

/* Real kernel variable, check it is in the right place */
extern volatile u64_t _sys_clock_tick_count;

struct test_struct app_data = {3, 4, NULL};
struct test_struct app_bss;
struct test_struct __noinit app_noinit;

int data_loc(char *start, char *end, void *ptr)
{
	if ((char *)ptr >= start && (char *)ptr < end) {
		return 1;
	}

	printk("Address %p outside range %p - %p\n", ptr, start, end);
	return 0;
}

int app_loc(void *ptr)
{
	return data_loc(__app_ram_start, __app_ram_end, ptr);
}

int kernel_loc(void *ptr)
{
	return data_loc(__kernel_ram_start, __kernel_ram_end, ptr);
}

/**
 * @brief Test to determine the memory bounds
 *
 * @ingroup kernel_memprotect_tests
 */
void test_app_memory(void)
{
	printk("Memory bounds:\n");
	printk("Application  %p - %p\n", __app_ram_start, __app_ram_end);
	printk("Kernel       %p - %p\n", __kernel_ram_start, __kernel_ram_end);

	zassert_true(app_loc(&app_data), "not in app memory");
	zassert_true(app_loc(&app_bss), "not in app memory");
	zassert_true(app_loc(&app_noinit), "not in app memory");

	zassert_true(kernel_loc(&kernel_data), "not in kernel memory");
	zassert_true(kernel_loc(&kernel_bss), "not in kernel memory");
	zassert_true(kernel_loc(&kernel_noinit), "not in kernel memory");

	zassert_true(kernel_loc((void *)&_sys_clock_tick_count),
		     "not in kernel memory");
}

void test_main(void)
{
	ztest_test_suite(app_memory,
			 ztest_unit_test(test_app_memory));

	ztest_run_test_suite(app_memory);
}
