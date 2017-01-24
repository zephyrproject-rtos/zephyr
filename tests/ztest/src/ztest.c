/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdio.h>

enum {
	TEST_PHASE_SETUP,
	TEST_PHASE_TEST,
	TEST_PHASE_TEARDOWN,
	TEST_PHASE_FRAMEWORK
} phase = TEST_PHASE_FRAMEWORK;

static int test_status;


static int cleanup_test(struct unit_test *test)
{
	int ret = TC_PASS;
	int mock_status;

	mock_status = _cleanup_mock();
	if (!ret && mock_status == 1) {
		PRINT("Test %s failed: Unused mock parameter values\n",
		      test->name);
		ret = TC_FAIL;
	} else if (!ret && mock_status == 2) {
		PRINT("Test %s failed: Unused mock return values\n",
		      test->name);
		ret = TC_FAIL;
	}

	return ret;
}

static void run_test_functions(struct unit_test *test)
{
	phase = TEST_PHASE_SETUP;
	test->setup();
	phase = TEST_PHASE_TEST;
	test->test();
	phase = TEST_PHASE_TEARDOWN;
	test->teardown();
	phase = TEST_PHASE_FRAMEWORK;
}

#ifndef KERNEL
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define FAIL_FAST 0

static jmp_buf test_fail;
static jmp_buf stack_fail;

void ztest_test_fail(void)
{
	raise(SIGABRT);
}

static void handle_signal(int sig)
{
	static const char *const phase_str[] = {
		"setup",
		"unit test",
		"teardown",
	};

	PRINT("    %s", strsignal(sig));
	switch (phase) {
	case TEST_PHASE_SETUP:
	case TEST_PHASE_TEST:
	case TEST_PHASE_TEARDOWN:
		PRINT(" at %s function\n", phase_str[phase]);
		longjmp(test_fail, 1);
	case TEST_PHASE_FRAMEWORK:
		PRINT("\n");
		longjmp(stack_fail, 1);
	}
}

static void init_testing(void)
{
	signal(SIGABRT, handle_signal);
	signal(SIGSEGV, handle_signal);

	if (setjmp(stack_fail)) {
		PRINT("Test suite crashed.");
		exit(1);
	}
}

static int run_test(struct unit_test *test)
{
	int ret = TC_PASS;

	TC_START(test->name);

	if (setjmp(test_fail)) {
		ret = TC_FAIL;
		goto out;
	}

	run_test_functions(test);
out:
	ret |= cleanup_test(test);
	_TC_END_RESULT(ret, test->name);

	return ret;
}

#else /* KERNEL */

/* Zephyr's probably going to cause all tests to fail if one test fails, so
 * skip the rest of tests if one of them fails
 */
#ifdef CONFIG_ZTEST_FAIL_FAST
#define FAIL_FAST 1
#else
#define FAIL_FAST 0
#endif

#if CONFIG_ZTEST_STACKSIZE & (STACK_ALIGN - 1)
    #error "CONFIG_ZTEST_STACKSIZE must be a multiple of the stack alignment"
#endif
static char __stack thread_stack[CONFIG_ZTEST_STACKSIZE];

static int test_result;
static struct k_sem mutex;

void ztest_test_fail(void)
{
	test_result = -1;
	k_sem_give(&mutex);
	k_thread_abort(k_current_get());
}

static void init_testing(void)
{
	k_sem_init(&mutex, 0, 1);
}

static void test_cb(void *a, void *dummy2, void *dummy)
{
	struct unit_test *test = (struct unit_test *)a;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy);

	test_result = 1;
	run_test_functions(test);
	test_result = 0;

	k_sem_give(&mutex);
}

static int run_test(struct unit_test *test)
{
	int ret = TC_PASS;

	TC_START(test->name);
	k_thread_spawn(&thread_stack[0], sizeof(thread_stack),
			 (k_thread_entry_t) test_cb, (struct unit_test *)test, NULL, NULL, -1, 0, 0);

	k_sem_take(&mutex, K_FOREVER);
	if (test_result) {
		ret = TC_FAIL;
	}

	if (!test_result || !FAIL_FAST) {
		ret |= cleanup_test(test);
	}

	_TC_END_RESULT(ret, test->name);

	return ret;
}

#endif /* !KERNEL */

void _ztest_run_test_suite(const char *name, struct unit_test *suite)
{
	int fail = 0;

	if (test_status < 0) {
		return;
	}

	init_testing();

	PRINT("Running test suite %s\n", name);
	while (suite->test) {
		fail += run_test(suite);
		suite++;

		if (fail && FAIL_FAST) {
			break;
		}
	}
	if (fail) {
		TC_END_REPORT(TC_FAIL);
	} else {
		TC_END_REPORT(TC_PASS);
	}
	test_status = (test_status || fail) ? 1 : 0;
}

void test_main(void);

#ifndef KERNEL
int main(void)
{
	_init_mock();
	test_main();

	return test_status;
}
#else
void main(void)
{
	_init_mock();
	test_main();
}
#endif
