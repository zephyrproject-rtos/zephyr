/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr/kernel.h>
#include <zephyr/toolchain.h>
#include <zephyr/ztest.h>


#define STACKSIZE       (2048 + CONFIG_TEST_EXTRA_STACK_SIZE)

ZTEST_BMEM static int count;
ZTEST_BMEM static int ret = TC_PASS;

void k_sys_fatal_error_handler(unsigned int reason, const struct arch_esf *esf)
{
	if (reason != K_ERR_STACK_CHK_FAIL) {
		printk("wrong error type\n");
		TC_END_REPORT(TC_FAIL);
		k_fatal_halt(reason);
	}
}

void check_input(const char *name, const char *input);

/**
 *
 * print_loop
 *
 * This function calls check_input 6 times with the input name and a short
 * string, which is printed properly by check_input.
 *
 * @param name    caller identification string
 *
 */

void print_loop(const char *name)
{
	while (count < 6) {
		/* A short input string to check_input.  It will pass. */
		check_input(name, "Stack ok");
		count++;
	}
}

/**
 *
 * check_input
 *
 * This function copies the input string to a buffer of 16 characters and
 * prints the name and buffer as a string.  If the input string is longer
 * than the buffer, an error condition is detected.
 *
 * When stack protection feature is enabled (see prj.conf file), the
 * system error handler is invoked and reports a "Stack Check Fail" error.
 * When stack protection feature is not enabled, the system crashes with
 * error like: Trying to execute code outside RAM or ROM.
 *
 */

void __noinline check_input(const char *name, const char *input)
{
	/* Stack will overflow when input is more than 16 characters */
	char buf[16];

	strcpy(buf, input);
	TC_PRINT("%s: %s\n", name, buf);
}

/**
 *
 * This thread passes a long string to check_input function.  It terminates due
 * to stack overflow and reports "Stack Check Fail" when stack protection
 * feature is enabled.  Hence it will not execute the print_loop function
 * and will not set ret to TC_FAIL.
 *
 */
void alternate_thread(void *p1, void *p2, void *p3)
{
	ARG_UNUSED(p1);
	ARG_UNUSED(p2);
	ARG_UNUSED(p3);
	/*
	 * Padding buffer to absorb the intentional overflow inside the thread stack.
	 * This prevents writes from crossing the thread stack boundary into the next
	 * MPU-protected region. Required to make the test independent of compiler-
	 * specific stack frame layouts.
	 */
	volatile __unused char overflow_guard_area[32] = "Forcing Initialization!";

	TC_PRINT("Starts %s\n", __func__);
	check_input(__func__,
		    "Input string is too long and stack overflowed!\n");
	/*
	 * Expect this thread to terminate due to stack check fail and will not
	 * execute pass here.
	 */
	print_loop(__func__);

	ret = TC_FAIL;
}



K_THREAD_STACK_DEFINE(alt_thread_stack_area, STACKSIZE);
static struct k_thread alt_thread_data;

/**
 * @brief Verify that legitimate stack use does not trip the canary.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * The stack protector must be silent for well-behaved code: only an actual
 * overflow may trigger it. This case runs the same buffer-handling function
 * the overflow case uses, but with input that fits, and also checks that the
 * overflow case's failure flag was never set -- the overflowing thread must
 * have been stopped by the canary before it could reach the code that sets
 * the flag. Ztest orders a suite alphabetically, so the overflow case
 * (test_stackprot_canary_overflow) has already run when this check is made.
 *
 * Test steps:
 * - Check that the shared failure flag is still clear.
 * - Call the buffer-copying function repeatedly with input that fits.
 *
 * Expected result:
 * - The calls complete normally and no stack check failure is raised.
 */
ZTEST_USER(stackprot, test_stackprot_no_false_positive)
{
	zassert_true(ret == TC_PASS);
	print_loop(__func__);
}

/**
 * @brief Verify that a stack buffer overflow is caught by the canary.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * With CONFIG_STACK_CANARIES a function whose local buffer is overrun must be
 * stopped when it returns, raising K_ERR_STACK_CHK_FAIL rather than letting
 * corrupted stack contents be used. The overflowing thread would set a shared
 * failure flag if it survived, and the suite's fatal error handler accepts
 * only the stack-check reason, so both a missed overflow and a wrong error
 * type are detected.
 *
 * Test steps:
 * - Spawn a thread that copies an over-long string into a 16-byte stack
 *   buffer.
 * - Let it run; the canary check fails when the overflowed function returns.
 * - The fatal error handler verifies the reason is K_ERR_STACK_CHK_FAIL.
 *
 * Expected result:
 * - The thread is terminated by the stack check and never reaches the code
 *   that would flag a failure.
 */
ZTEST(stackprot, test_stackprot_canary_overflow)
{
	/* Start thread */
	k_thread_create(&alt_thread_data, alt_thread_stack_area, STACKSIZE,
			alternate_thread, NULL, NULL, NULL,
			K_PRIO_COOP(1), K_USER, K_NO_WAIT);

	/* Note that this sleep is required on SMP platforms where
	 * that thread will execute asynchronously!
	 */
	k_sleep(K_MSEC(100));
}

#ifdef CONFIG_STACK_CANARIES_TLS
extern Z_THREAD_LOCAL volatile uintptr_t __stack_chk_guard;
#else
extern volatile uintptr_t __stack_chk_guard;
#endif

/**
 * This thread checks its canary value against its parent canary.
 * If CONFIG_STACK_CANARIES_TLS is enabled, it is expected that the
 * canaries have different values, otherwise there is only one global
 * canary and the value should be the same.
 */
void alternate_thread_canary(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);

	TC_PRINT("Starts %s\n", __func__);

#ifdef CONFIG_STACK_CANARIES_TLS
	zassert_false(__stack_chk_guard == (uintptr_t)arg1);
#else
	zassert_true(__stack_chk_guard == (uintptr_t)arg1);
#endif
}

/**
 * @brief Verify the scope of the stack canary value.
 *
 * @ingroup kernel_memprotect_tests
 *
 * @details
 * With CONFIG_STACK_CANARIES_TLS every thread gets its own canary value, so
 * leaking one thread's canary does not help overflow another's; without it
 * there is a single global canary shared by all threads. The spawned thread
 * compares its own canary against its parent's, and which relation is correct
 * depends on the configuration under test.
 *
 * Test steps:
 * - Read the current thread's canary value.
 * - Spawn a thread, passing it that value, and have it compare against its
 *   own canary.
 *
 * Expected result:
 * - With per-thread canaries the values differ; with a global canary they are
 *   identical.
 */
ZTEST(stackprot, test_stackprot_canary_scope)
{
	/* Start thread */
	k_thread_create(&alt_thread_data, alt_thread_stack_area, STACKSIZE,
			alternate_thread_canary,
			(void *)__stack_chk_guard, NULL, NULL,
			K_PRIO_COOP(1), K_USER, K_NO_WAIT);

	/* Note that this sleep is required on SMP platforms where
	 * that thread will execute asynchronously!
	 */
	k_sleep(K_MSEC(100));
}

ZTEST_SUITE(stackprot, NULL, NULL, NULL, NULL, NULL);
