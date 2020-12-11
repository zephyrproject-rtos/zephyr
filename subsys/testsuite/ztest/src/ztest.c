/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <stdio.h>
#include <app_memory/app_memdomain.h>
#ifdef CONFIG_USERSPACE
#include <sys/libc-hooks.h>
#endif
#include <power/reboot.h>

#ifdef KERNEL
static struct k_thread ztest_thread;
#endif

#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#endif

/* ZTEST_DMEM and ZTEST_BMEM are used for the application shared memory test  */

ZTEST_DMEM enum {
	TEST_PHASE_SETUP,
	TEST_PHASE_TEST,
	TEST_PHASE_TEARDOWN,
	TEST_PHASE_FRAMEWORK
} phase = TEST_PHASE_FRAMEWORK;

static ZTEST_BMEM int test_status;

/**
 * @brief Try to shorten a filename by removing the current directory
 *
 * This helps to reduce the very long filenames in assertion failures. It
 * removes the current directory from the filename and returns the rest.
 * This makes assertions a lot more readable, and sometimes they fit on one
 * line.
 *
 * @param file Filename to check
 * @returns Shortened filename, or @file if it could not be shortened
 */
const char *ztest_relative_filename(const char *file)
{
#ifdef CONFIG_ARCH_POSIX
	const char *cwd;
	char buf[200];

	cwd = getcwd(buf, sizeof(buf));
	if (cwd && strlen(file) > strlen(cwd) &&
	    !strncmp(file, cwd, strlen(cwd)))
		return file + strlen(cwd) + 1; /* move past the trailing '/' */
#endif
	return file;
}

static int cleanup_test(struct unit_test *test)
{
	int ret = TC_PASS;
	int mock_status;

	mock_status = z_cleanup_mock();

#ifdef KERNEL
	/* we need to remove the ztest_thread information from the timeout_q.
	 * Because we reuse the same k_thread structure this would
	 * causes some problems.
	 */
	k_thread_abort(&ztest_thread);
#endif

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

#ifdef KERNEL
#ifdef CONFIG_SMP
#define NUM_CPUHOLD (CONFIG_MP_NUM_CPUS - 1)
#else
#define NUM_CPUHOLD 0
#endif
#define CPUHOLD_STACK_SZ (512 + CONFIG_TEST_EXTRA_STACKSIZE)

static struct k_thread cpuhold_threads[NUM_CPUHOLD];
K_KERNEL_STACK_ARRAY_DEFINE(cpuhold_stacks, NUM_CPUHOLD, CPUHOLD_STACK_SZ);
static struct k_sem cpuhold_sem;
volatile int cpuhold_active;

/* "Holds" a CPU for use with the "1cpu" test cases.  Note that we
 * can't use tools like the cpumask feature because we have tests that
 * may need to control that configuration themselves.  We do this at
 * the lowest level, but locking interrupts directly and spinning.
 */
static void cpu_hold(void *arg1, void *arg2, void *arg3)
{
	ARG_UNUSED(arg1);
	ARG_UNUSED(arg2);
	ARG_UNUSED(arg3);
	unsigned int key = arch_irq_lock();
	uint32_t dt, start_ms = k_uptime_get_32();

	k_sem_give(&cpuhold_sem);

	while (cpuhold_active) {
		k_busy_wait(1000);
	}

	/* Holding the CPU via spinning is expensive, and abusing this
	 * for long-running test cases tends to overload the CI system
	 * (qemu runs separate CPUs in different threads, but the CI
	 * logic views it as one "job") and cause other test failures.
	 */
	dt = k_uptime_get_32() - start_ms;
	zassert_true(dt < 3000,
		     "1cpu test took too long (%d ms)", dt);
	arch_irq_unlock(key);
}

void z_impl_z_test_1cpu_start(void)
{
	cpuhold_active = 1;
#ifdef CONFIG_THREAD_NAME
	char tname[CONFIG_THREAD_MAX_NAME_LEN];
#endif
	k_sem_init(&cpuhold_sem, 0, 999);

	/* Spawn N-1 threads to "hold" the other CPUs, waiting for
	 * each to signal us that it's locked and spinning.
	 *
	 * Note that NUM_CPUHOLD can be a value that causes coverity
	 * to flag the following loop as DEADCODE so suppress the warning.
	 */
	/* coverity[DEADCODE] */
	for (int i = 0; i < NUM_CPUHOLD; i++)  {
		k_thread_create(&cpuhold_threads[i],
				cpuhold_stacks[i], CPUHOLD_STACK_SZ,
				(k_thread_entry_t) cpu_hold, NULL, NULL, NULL,
				K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);
#ifdef CONFIG_THREAD_NAME
		snprintk(tname, CONFIG_THREAD_MAX_NAME_LEN, "cpuhold%02d", i);
		k_thread_name_set(&cpuhold_threads[i], tname);
#endif
		k_sem_take(&cpuhold_sem, K_FOREVER);
	}
}

void z_impl_z_test_1cpu_stop(void)
{
	cpuhold_active = 0;

	/* Note that NUM_CPUHOLD can be a value that causes coverity
	 * to flag the following loop as DEADCODE so suppress the warning.
	 */
	/* coverity[DEADCODE] */
	for (int i = 0; i < NUM_CPUHOLD; i++)  {
		k_thread_abort(&cpuhold_threads[i]);
	}
}

#ifdef CONFIG_USERSPACE
void z_vrfy_z_test_1cpu_start(void)
{
	z_impl_z_test_1cpu_start();
}
#include <syscalls/z_test_1cpu_start_mrsh.c>

void z_vrfy_z_test_1cpu_stop(void)
{
	z_impl_z_test_1cpu_stop();
}
#include <syscalls/z_test_1cpu_stop_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif

static void run_test_functions(struct unit_test *test)
{
	phase = TEST_PHASE_SETUP;
	test->setup();
	phase = TEST_PHASE_TEST;
	test->test();
}

#ifndef KERNEL
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>

#define FAIL_FAST 0

static jmp_buf test_fail;
static jmp_buf test_pass;
static jmp_buf stack_fail;

void ztest_test_fail(void)
{
	raise(SIGABRT);
}

void ztest_test_pass(void)
{
	longjmp(test_pass, 1);
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

	if (setjmp(test_pass)) {
		ret = TC_PASS;
		goto out;
	}

	run_test_functions(test);
out:
	ret |= cleanup_test(test);
	Z_TC_END_RESULT(ret, test->name);

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

K_THREAD_STACK_DEFINE(ztest_thread_stack, CONFIG_ZTEST_STACKSIZE +
		      CONFIG_TEST_EXTRA_STACKSIZE);
static ZTEST_BMEM int test_result;

void ztest_test_fail(void)
{
	test_result = -1;
	k_thread_abort(&ztest_thread);
	k_thread_abort(k_current_get());
}

void ztest_test_pass(void)
{
	test_result = 0;
	k_thread_abort(&ztest_thread);
	k_thread_abort(k_current_get());
}

void ztest_test_skip(void)
{
	test_result = -2;
	k_thread_abort(&ztest_thread);
	k_thread_abort(k_current_get());
}

static void init_testing(void)
{
	k_object_access_all_grant(&ztest_thread);
}

static void test_cb(void *a, void *dummy2, void *dummy)
{
	struct unit_test *test = (struct unit_test *)a;

	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy);

	test_result = 1;
	run_test_functions(test);
	test_result = 0;
}

static int run_test(struct unit_test *test)
{
	int ret = TC_PASS;

	TC_START(test->name);
	k_thread_create(&ztest_thread, ztest_thread_stack,
			K_THREAD_STACK_SIZEOF(ztest_thread_stack),
			(k_thread_entry_t) test_cb, (struct unit_test *)test,
			NULL, NULL, CONFIG_ZTEST_THREAD_PRIORITY,
			test->thread_options | K_INHERIT_PERMS,
				K_NO_WAIT);

	k_thread_name_set(&ztest_thread, "ztest_thread");
	k_thread_join(&ztest_thread, K_FOREVER);

	phase = TEST_PHASE_TEARDOWN;
	test->teardown();
	phase = TEST_PHASE_FRAMEWORK;

	if (test_result == -1) {
		ret = TC_FAIL;
	}

	if (!test_result || !FAIL_FAST) {
		ret |= cleanup_test(test);
	}

	if (test_result == -2) {
		Z_TC_END_RESULT(TC_SKIP, test->name);
	} else {
		Z_TC_END_RESULT(ret, test->name);
	}

	return ret;
}

#endif /* !KERNEL */

void z_ztest_run_test_suite(const char *name, struct unit_test *suite)
{
	int fail = 0;

	if (test_status < 0) {
		return;
	}

	init_testing();

	PRINT("Running test suite %s\n", name);
	PRINT_LINE;
	while (suite->test) {
		fail += run_test(suite);
		suite++;

		if (fail && FAIL_FAST) {
			break;
		}
	}
	if (fail) {
		TC_PRINT("Test suite %s failed.\n", name);
	} else {
		TC_PRINT("Test suite %s succeeded\n", name);
	}

	test_status = (test_status || fail) ? 1 : 0;
}

void end_report(void)
{
	if (test_status) {
		TC_END_REPORT(TC_FAIL);
	} else {
		TC_END_REPORT(TC_PASS);
	}
}

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(ztest_mem_partition);
#endif

#ifndef KERNEL
int main(void)
{
	z_init_mock();
	test_main();
	end_report();

	return test_status;
}
#else
void main(void)
{
#ifdef CONFIG_USERSPACE
	/* Partition containing globals tagged with ZTEST_DMEM and ZTEST_BMEM
	 * macros. Any variables that user code may reference need to be
	 * placed in this partition if no other memory domain configuration
	 * is made.
	 */
	k_mem_domain_add_partition(&k_mem_domain_default,
				   &ztest_mem_partition);
#ifdef Z_MALLOC_PARTITION_EXISTS
	/* Allow access to malloc() memory */
	k_mem_domain_add_partition(&k_mem_domain_default,
				   &z_malloc_partition);
#endif
#endif /* CONFIG_USERSPACE */

	z_init_mock();
	test_main();
	end_report();
	if (IS_ENABLED(CONFIG_ZTEST_RETEST_IF_PASSED)) {
		static __noinit struct {
			uint32_t magic;
			uint32_t boots;
		} state;
		const uint32_t magic = 0x152ac523;

		if (state.magic != magic) {
			state.magic = magic;
			state.boots = 0;
		}
		state.boots += 1;
		if (test_status == 0) {
			PRINT("Reset board #%u to test again\n",
				state.boots);
			k_msleep(10);
			sys_reboot(SYS_REBOOT_COLD);
		} else {
			PRINT("Failed after %u attempts\n", state.boots);
			state.boots = 0;
		}
	}
}
#endif
