/*
 * Parts derived from tests/kernel/fatal/src/main.c, which has the
 * following copyright and license:
 *
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>
#include <zephyr/sys/barrier.h>
#include <zephyr/toolchain.h>
#include <string.h>
#include <stdlib.h>

#include "targets.h"

/* 32-bit IA32 page tables have no mechanism to restrict execution */
#if defined(CONFIG_X86) && !defined(CONFIG_X86_64) && !defined(CONFIG_X86_PAE)
#define SKIP_EXECUTE_TESTS
#endif

/* RISC-V does not always have a mechanism to restrict execution */
#if defined(CONFIG_RISCV) && !defined(CONFIG_PMP_DATA_EXECUTION_PREVENTION)
#define SKIP_EXECUTE_TESTS
#endif

#define INFO(fmt, ...) printk(fmt, ##__VA_ARGS__)

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *pEsf)
{
	INFO("Caught system error -- reason %d\n", reason);
	ztest_test_pass();
}

#ifdef CONFIG_COMPILER_ISA_THUMB2
/* Must clear LSB of function address to access as data. */
#define FUNC_TO_PTR(x) (void *)((uintptr_t)(x) & ~0x1)
/* Must set LSB of function address to call in Thumb mode. */
#define PTR_TO_FUNC(x) (int (*)(int))((uintptr_t)(x) | 0x1)
/* Flush preceding data writes and instruction fetches. */
#define DO_BARRIERS() do { barrier_dsync_fence_full(); \
			   barrier_isync_fence_full(); \
			} while (0)
#else
#define FUNC_TO_PTR(x) (void *)(x)
#define PTR_TO_FUNC(x) (int (*)(int))(x)
#define DO_BARRIERS() do { } while (0)
#endif

static int __noinline add_one(int i)
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
 * @brief Verify that writing to .rodata faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Read-only data must be mapped without write permission, so an attempt to
 * modify it has to trap rather than silently succeed.
 *
 * The suite installs a fatal error handler that reports the case as passed
 * and moves on, so a caught fault is the success path: reaching the
 * zassert_unreachable() after the access means the region was not protected.
 *
 * Test steps:
 * - Take a pointer to a variable in .rodata.
 * - Write the inverse of its value through that pointer.
 *
 * Expected result:
 * - The write raises a fatal error; the code after it is never reached.
 */
ZTEST(protection, test_protection_write_rodata)
{
	volatile uint32_t *ptr = (volatile uint32_t *)&rodata_var;

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
 * @brief Verify that writing to .text faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Executable code must be mapped without write permission, otherwise running
 * code could be rewritten in place. The test copies one function over
 * another, which is the shape a real code-injection attempt would take.
 *
 * The suite installs a fatal error handler that reports the case as passed
 * and moves on, so a caught fault is the success path: reaching the
 * zassert_unreachable() after the access means the region was not protected.
 *
 * Test steps:
 * - Copy the body of one function over another function in .text.
 * - Call the overwritten function.
 *
 * Expected result:
 * - The write raises a fatal error; the code after it is never reached.
 */
ZTEST(protection, test_protection_write_text)
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
 * @brief Verify that executing from .data faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Writable data must be mapped without execute permission, so code copied
 * into a data buffer cannot be run. Skipped on configurations without data
 * execution prevention, where the hardware cannot enforce this.
 *
 * The suite installs a fatal error handler that reports the case as passed
 * and moves on, so a caught fault is the success path: reaching the
 * zassert_unreachable() after the access means the region was not protected.
 *
 * Test steps:
 * - Copy a small function into a buffer in .data.
 * - Call into that buffer.
 *
 * Expected result:
 * - The call raises a fatal error; the code after it is never reached.
 */
ZTEST(protection, test_protection_exec_data)
{
#ifdef SKIP_EXECUTE_TESTS
	ztest_test_skip();
#else
	execute_from_buffer(data_buf);
	zassert_unreachable("Execute from data did not fault");
#endif
}

/**
 * @brief Verify that executing from the stack faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * A thread stack must be mapped without execute permission, which is what
 * stops a stack overflow from being turned into code execution. Skipped on
 * configurations without data execution prevention.
 *
 * The suite installs a fatal error handler that reports the case as passed
 * and moves on, so a caught fault is the success path: reaching the
 * zassert_unreachable() after the access means the region was not protected.
 *
 * Test steps:
 * - Copy a small function into a buffer on the current thread's stack.
 * - Call into that buffer.
 *
 * Expected result:
 * - The call raises a fatal error; the code after it is never reached.
 */
ZTEST(protection, test_protection_exec_stack)
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
 * @brief Verify that executing from the heap faults.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * Heap memory must be mapped without execute permission, for the same reason
 * as the stack and data cases. Skipped when there is no heap configured or
 * the platform has no data execution prevention.
 *
 * The suite installs a fatal error handler that reports the case as passed
 * and moves on, so a caught fault is the success path: reaching the
 * zassert_unreachable() after the access means the region was not protected.
 *
 * Test steps:
 * - Allocate a buffer with k_malloc() and copy a small function into it.
 * - Call into that buffer.
 *
 * Expected result:
 * - The call raises a fatal error; the code after it is never reached.
 *
 * @see k_malloc()
 */
ZTEST(protection, test_protection_exec_heap)
{
#if (CONFIG_HEAP_MEM_POOL_SIZE > 0) && !defined(SKIP_EXECUTE_TESTS)
	uint8_t *heap_buf = k_malloc(BUF_SIZE);

	execute_from_buffer(heap_buf);
	k_free(heap_buf);
	zassert_unreachable("Execute from heap did not fault");
#else
	ztest_test_skip();
#endif
}

ZTEST_SUITE(protection, NULL, NULL, NULL, NULL, NULL);
