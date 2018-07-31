/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */


#include <zephyr.h>
#include <ztest.h>


#define STACKSIZE       2048
#define PRIORITY        5

static int count;
static int ret = TC_PASS;

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
 * @return N/A
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
 * @return N/A
 */

void check_input(const char *name, const char *input)
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
 * @return N/A
 */
void alternate_thread(void)
{
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
 * @brief test Stack Protector feature using canary
 *
 * @details This is the test program to test stack protection using canary.
 * The main thread starts a second thread, which generates a stack check
 * failure.
 * By design, the second thread will not complete its execution and
 * will not set ret to TC_FAIL.
 * This is the entry point to the test stack protection feature.
 * It starts the thread that tests stack protection, then prints out
 * a few messages before terminating.
 *
 * @ingroup kernel_memprotect_tests
 */

void test_stackprot(void)
{
	zassert_true(ret == TC_PASS, NULL);
	print_loop(__func__);
}

void test_main(void)
{
	/* Start thread */
	k_thread_create(&alt_thread_data, alt_thread_stack_area, STACKSIZE,
			(k_thread_entry_t)alternate_thread, NULL, NULL, NULL,
			K_PRIO_PREEMPT(PRIORITY), 0, K_NO_WAIT);

	ztest_test_suite(stackprot,
			 ztest_unit_test(test_stackprot));
	ztest_run_test_suite(stackprot);
}
