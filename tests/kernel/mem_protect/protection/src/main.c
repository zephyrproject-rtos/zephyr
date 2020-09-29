/*
 * Parts derived from tests/kernel/fatal/src/main.c, which has the
 * following copyright and license:
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <ztest.h>
#include <kernel_structs.h>
#include <string.h>
#include <stdlib.h>

#include "targets.h"

/* 32-bit IA32 page tables have no mechanism to restrict execution */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && !defined(CONFIG_X86_PAE)
#define SKIP_EXECUTE_TESTS
#endif

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)

void k_sys_fatal_error_handler(unsigned int reason, const z_arch_esf_t *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);
	ztest_test_pass();
}

#ifdef CONFIG_CPU_CORTEX_M
#include <arch/arm/aarch32/cortex_m/cmsis.h>
/* Must clear LSB of function address to access as data. */
#define FUNC_TO_PTR(x) (void *)((uintptr_t)(x) & ~0x1)
/* Must set LSB of function address to call in Thumb mode. */
#define PTR_TO_FUNC(x) (int (*)(int))((uintptr_t)(x) | 0x1)
/* Flush preceding data writes and instruction fetches. */
#define DO_BARRIERS() do { __DSB(); __ISB(); } while (0)
#else
#define FUNC_TO_PTR(x) (void *)(x)
#define PTR_TO_FUNC(x) (int (*)(int))(x)
#define DO_BARRIERS() do { } while (0)
#endif

static int __attribute__((noinline)) add_one(int i)
{
	return (i + 1);
}

#ifndef SKIP_EXECUTE_TESTS
static void execute_from_buffer(uint8_t *dst)
{
	void *src = FUNC_TO_PTR(add_one);
	int (*func)(int i) = PTR_TO_FUNC(dst);
	int i = 1;

	/* Copy add_one() code to destination buffer. */
	memcpy(dst, src, BUF_SIZE);
	DO_BARRIERS();

	/*
	 * Try executing from buffer we just filled.
	 * Optimally, this triggers a fault.
	 * If not, we check to see if the function
	 * returned the expected result as confirmation
	 * that we truly executed the code we wrote.
	 */
	INFO("trying to call code written to %p\n", func);
	i = func(i);
	INFO("returned from code at %p\n", func);
	if (i == 2) {
		INFO("Execute from target buffer succeeded!\n");
	} else {
		INFO("Did not get expected return value!\n");
	}
}
#endif /* SKIP_EXECUTE_TESTS */

/**
 * @brief Test write to read only section
 *
 * @ingroup kernel_memprotect_tests
 */
static void test_write_ro(void)
{
	uint32_t *ptr = (uint32_t *)&rodata_var;

	/*
	 * Try writing to rodata.  Optimally, this triggers a fault.
	 * If not, we check to see if the rodata value actually changed.
	 */
	INFO("trying to write to rodata at %p\n", ptr);
	*ptr = ~RODATA_VALUE;

	DO_BARRIERS();

	if (*ptr == RODATA_VALUE) {
		INFO("rodata value still the same\n");
	} else if (*ptr == ~RODATA_VALUE) {
		INFO("rodata modified!\n");
	} else {
		INFO("something went wrong!\n");
	}

	zassert_unreachable("Write to rodata did not fault");
}

/**
 * @brief Test to execute on text section
 *
 * @ingroup kernel_memprotect_tests
 */
static void test_write_text(void)
{
	void *src = FUNC_TO_PTR(add_one);
	void *dst = FUNC_TO_PTR(overwrite_target);
	int i = 1;

	/*
	 * Try writing to a function in the text section.
	 * Optimally, this triggers a fault.
	 * If not, we try calling the function after overwriting
	 * to see if it returns the expected result as
	 * confirmation that we truly executed the code we wrote.
	 */
	INFO("trying to write to text at %p\n", dst);
	memcpy(dst, src, BUF_SIZE);
	DO_BARRIERS();
	i = overwrite_target(i);
	if (i == 2) {
		INFO("Overwrite of text succeeded!\n");
	} else {
		INFO("Did not get expected return value!\n");
	}

	zassert_unreachable("Write to text did not fault");
}

/**
 * @brief Test execution from data section
 *
 * @ingroup kernel_memprotect_tests
 */
static void test_exec_data(void)
{
#ifdef SKIP_EXECUTE_TESTS
	ztest_test_skip();
#else
	execute_from_buffer(data_buf);
	zassert_unreachable("Execute from data did not fault");
#endif
}

/**
 * @brief Test execution from stack section
 *
 * @ingroup kernel_memprotect_tests
 */
static void test_exec_stack(void)
{
#ifdef SKIP_EXECUTE_TESTS
	ztest_test_skip();
#else
	uint8_t stack_buf[BUF_SIZE] __aligned(sizeof(int));

	execute_from_buffer(stack_buf);
	zassert_unreachable("Execute from stack did not fault");
#endif
}

/**
 * @brief Test execution from heap
 *
 * @ingroup kernel_memprotect_tests
 */
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0) && !defined(SKIP_EXECUTE_TESTS)
static void test_exec_heap(void)
{
	uint8_t *heap_buf = k_malloc(BUF_SIZE);

	execute_from_buffer(heap_buf);
	k_free(heap_buf);
	zassert_unreachable("Execute from heap did not fault");
}
#else
static void test_exec_heap(void)
{
	ztest_test_skip();
}
#endif

void test_main(void)
{
	ztest_test_suite(protection,
			 ztest_unit_test(test_exec_data),
			 ztest_unit_test(test_exec_stack),
			 ztest_unit_test(test_exec_heap),
			 ztest_unit_test(test_write_ro),
			 ztest_unit_test(test_write_text)
		);
	ztest_run_test_suite(protection);
}
