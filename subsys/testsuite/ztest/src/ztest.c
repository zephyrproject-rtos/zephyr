/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdlib.h>
#include <zephyr/ztest.h>

#include <zephyr/app_memory/app_memdomain.h>
#ifdef CONFIG_USERSPACE
#include <zephyr/sys/libc-hooks.h>
#endif
#include <zephyr/logging/log_ctrl.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/llext/symbol.h>

#include <zephyr/sys/barrier.h>

#ifdef KERNEL
static struct k_thread ztest_thread;
#endif
static bool failed_expectation;

#ifdef CONFIG_ZTEST_SHELL
#include <zephyr/shell/shell.h>
#endif

#ifdef CONFIG_ZTEST_SHUFFLE
#include <time.h>
#include <zephyr/random/random.h>
#ifndef CONFIG_ZTEST_REPEAT
#define NUM_ITER_PER_SUITE CONFIG_ZTEST_SHUFFLE_SUITE_REPEAT_COUNT
#define NUM_ITER_PER_TEST  CONFIG_ZTEST_SHUFFLE_TEST_REPEAT_COUNT
#endif
#endif /* CONFIG_ZTEST_SHUFFLE */

#ifdef CONFIG_ZTEST_REPEAT
#define NUM_ITER_PER_SUITE CONFIG_ZTEST_SUITE_REPEAT_COUNT
#define NUM_ITER_PER_TEST  CONFIG_ZTEST_TEST_REPEAT_COUNT
#else
#ifndef CONFIG_ZTEST_SHUFFLE
#define NUM_ITER_PER_SUITE 1
#define NUM_ITER_PER_TEST  1
#endif
#endif

#ifdef CONFIG_ZTEST_COVERAGE_RESET_BEFORE_TESTS
#include <coverage.h>
#endif

/* ZTEST_DMEM and ZTEST_BMEM are used for the application shared memory test  */

/**
 * @brief The current status of the test binary
 */
enum ztest_status {
	ZTEST_STATUS_OK,
	ZTEST_STATUS_HAS_FAILURE,
	ZTEST_STATUS_CRITICAL_ERROR
};

/**
 * @brief Tracks the current phase that ztest is operating in.
 */
ZTEST_DMEM enum ztest_phase cur_phase = TEST_PHASE_FRAMEWORK;

static ZTEST_BMEM enum ztest_status test_status = ZTEST_STATUS_OK;

extern ZTEST_DMEM const struct ztest_arch_api ztest_api;

static void __ztest_show_suite_summary(void);

static void end_report(void)
{
	__ztest_show_suite_summary();
	if (test_status) {
		TC_END_REPORT(TC_FAIL);
	} else {
		TC_END_REPORT(TC_PASS);
	}
}

static int cleanup_test(struct ztest_unit_test *test)
{
	int ret = TC_PASS;
	int mock_status;

	mock_status = z_cleanup_mock();

#ifdef KERNEL
	/* we need to remove the ztest_thread information from the timeout_q.
	 * Because we reuse the same k_thread structure this would
	 * causes some problems.
	 */
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_thread_abort(&ztest_thread);
	}
#endif

	if (!ret && mock_status == 1) {
		PRINT_DATA("Test %s failed: Unused mock parameter values\n", test->name);
		ret = TC_FAIL;
	} else if (!ret && mock_status == 2) {
		PRINT_DATA("Test %s failed: Unused mock return values\n", test->name);
		ret = TC_FAIL;
	} else {
		;
	}

	return ret;
}

#ifdef KERNEL

#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
#define MAX_NUM_CPUHOLD  (CONFIG_MP_MAX_NUM_CPUS - 1)
#define CPUHOLD_STACK_SZ (512 + CONFIG_TEST_EXTRA_STACK_SIZE)

struct cpuhold_pool_item {
	struct k_thread thread;
	bool used;
};

static struct cpuhold_pool_item cpuhold_pool_items[MAX_NUM_CPUHOLD + 1];

K_KERNEL_STACK_ARRAY_DEFINE(cpuhold_stacks, MAX_NUM_CPUHOLD + 1, CPUHOLD_STACK_SZ);

static struct k_sem cpuhold_sem;

volatile int cpuhold_active;
volatile bool cpuhold_spawned;

static int find_unused_thread(void)
{
	for (unsigned int i = 0; i <= MAX_NUM_CPUHOLD; i++) {
		if (!cpuhold_pool_items[i].used) {
			return i;
		}
	}

	return -1;
}

static void mark_thread_unused(struct k_thread *thread)
{
	for (unsigned int i = 0; i <= MAX_NUM_CPUHOLD; i++) {
		if (&cpuhold_pool_items[i].thread == thread) {
			cpuhold_pool_items[i].used = false;
		}
	}
}

static inline void wait_for_thread_to_switch_out(struct k_thread *thread)
{
	unsigned int key = arch_irq_lock();
	volatile void **shp = (void *)&thread->switch_handle;

	while (*shp == NULL) {
		arch_spin_relax();
	}
	/* Read barrier: don't allow any subsequent loads in the
	 * calling code to reorder before we saw switch_handle go
	 * non-null.
	 */
	barrier_dmem_fence_full();

	arch_irq_unlock(key);
}

/* "Holds" a CPU for use with the "1cpu" test cases.  Note that we
 * can't use tools like the cpumask feature because we have tests that
 * may need to control that configuration themselves.  We do this at
 * the lowest level, but locking interrupts directly and spinning.
 */
static void cpu_hold(void *arg1, void *arg2, void *arg3)
{
	struct k_thread *thread = arg1;
	unsigned int idx = (unsigned int)(uintptr_t)arg2;
	char tname[CONFIG_THREAD_MAX_NAME_LEN];

	ARG_UNUSED(arg3);

	if (arch_proc_id() == 0) {
		int i;

		i = find_unused_thread();

		__ASSERT_NO_MSG(i != -1);

		cpuhold_spawned = false;

		cpuhold_pool_items[i].used = true;
		k_thread_create(&cpuhold_pool_items[i].thread, cpuhold_stacks[i], CPUHOLD_STACK_SZ,
				cpu_hold, k_current_get(), (void *)(uintptr_t)idx, NULL,
				K_HIGHEST_THREAD_PRIO, 0, K_NO_WAIT);

		/*
		 * Busy-wait until we know the spawned thread is running to
		 * ensure it does not spawn on CPU0.
		 */

		while (!cpuhold_spawned) {
			k_busy_wait(1000);
		}

		return;
	}

	if (thread != NULL) {
		cpuhold_spawned = true;

		/* Busywait until a new thread is scheduled in on CPU0 */

		wait_for_thread_to_switch_out(thread);

		mark_thread_unused(thread);
	}

	if (IS_ENABLED(CONFIG_THREAD_NAME)) {
		snprintk(tname, CONFIG_THREAD_MAX_NAME_LEN, "cpuhold%02d", idx);
		k_thread_name_set(k_current_get(), tname);
	}

	uint32_t dt, start_ms = k_uptime_get_32();
	unsigned int key = arch_irq_lock();

	k_sem_give(&cpuhold_sem);

#if (defined(CONFIG_ARM64) || defined(CONFIG_RISCV)) && defined(CONFIG_FPU_SHARING)
	/*
	 * We'll be spinning with IRQs disabled. The flush-your-FPU request
	 * IPI will never be serviced during that time. Therefore we flush
	 * the FPU preemptively here to prevent any other CPU waiting after
	 * this CPU forever and deadlock the system.
	 */
	k_float_disable(arch_curr_cpu()->arch.fpu_owner);
#endif

	while (cpuhold_active) {
		k_busy_wait(1000);
	}

	/* Holding the CPU via spinning is expensive, and abusing this
	 * for long-running test cases tends to overload the CI system
	 * (qemu runs separate CPUs in different threads, but the CI
	 * logic views it as one "job") and cause other test failures.
	 */
	dt = k_uptime_get_32() - start_ms;
	zassert_true(dt < CONFIG_ZTEST_CPU_HOLD_TIME_MS, "1cpu test took too long (%d ms)", dt);
	arch_irq_unlock(key);
}
#endif /* CONFIG_SMP && (CONFIG_MP_MAX_NUM_CPUS > 1) */

void z_impl_z_test_1cpu_start(void)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	unsigned int num_cpus = arch_num_cpus();
	int j;

	cpuhold_active = 1;

	k_sem_init(&cpuhold_sem, 0, 999);

	/* Spawn N-1 threads to "hold" the other CPUs, waiting for
	 * each to signal us that it's locked and spinning.
	 */
	for (int i = 0; i < num_cpus - 1; i++) {
		j = find_unused_thread();

		__ASSERT_NO_MSG(j != -1);

		cpuhold_pool_items[j].used = true;
		k_thread_create(&cpuhold_pool_items[j].thread, cpuhold_stacks[j], CPUHOLD_STACK_SZ,
				cpu_hold, NULL, (void *)(uintptr_t)i, NULL, K_HIGHEST_THREAD_PRIO,
				0, K_NO_WAIT);
		k_sem_take(&cpuhold_sem, K_FOREVER);
	}
#endif
}

void z_impl_z_test_1cpu_stop(void)
{
#if defined(CONFIG_SMP) && (CONFIG_MP_MAX_NUM_CPUS > 1)
	cpuhold_active = 0;

	for (int i = 0; i <= MAX_NUM_CPUHOLD; i++) {
		if (cpuhold_pool_items[i].used) {
			k_thread_abort(&cpuhold_pool_items[i].thread);
			cpuhold_pool_items[i].used = false;
		}
	}
#endif
}

#ifdef CONFIG_USERSPACE
void z_vrfy_z_test_1cpu_start(void)
{
	z_impl_z_test_1cpu_start();
}
#include <zephyr/syscalls/z_test_1cpu_start_mrsh.c>

void z_vrfy_z_test_1cpu_stop(void)
{
	z_impl_z_test_1cpu_stop();
}
#include <zephyr/syscalls/z_test_1cpu_stop_mrsh.c>
#endif /* CONFIG_USERSPACE */
#endif

__maybe_unused static void run_test_rules(bool is_before, struct ztest_unit_test *test, void *data)
{
	for (struct ztest_test_rule *rule = _ztest_test_rule_list_start;
	     rule < _ztest_test_rule_list_end; ++rule) {
		if (is_before && rule->before_each) {
			rule->before_each(test, data);
		} else if (!is_before && rule->after_each) {
			rule->after_each(test, data);
		}
	}
}

static void run_test_functions(struct ztest_suite_node *suite, struct ztest_unit_test *test,
			       void *data)
{
	__ztest_set_test_phase(TEST_PHASE_TEST);
	test->test(data);
}

COND_CODE_1(KERNEL, (ZTEST_BMEM), ()) static enum ztest_result test_result;

static int get_final_test_result(const struct ztest_unit_test *test, int ret)
{
	enum ztest_expected_result expected_result = -1;

	for (struct ztest_expected_result_entry *expectation =
		     _ztest_expected_result_entry_list_start;
	     expectation < _ztest_expected_result_entry_list_end; ++expectation) {
		if (strcmp(expectation->test_name, test->name) == 0 &&
		    strcmp(expectation->test_suite_name, test->test_suite_name) == 0) {
			expected_result = expectation->expected_result;
			break;
		}
	}

	if (expected_result == ZTEST_EXPECTED_RESULT_FAIL) {
		/* Expected a failure:
		 * - If we got a failure, return TC_PASS
		 * - Otherwise force a failure
		 */
		return (ret == TC_FAIL) ? TC_PASS : TC_FAIL;
	}
	if (expected_result == ZTEST_EXPECTED_RESULT_SKIP) {
		/* Expected a skip:
		 * - If we got a skip, return TC_PASS
		 * - Otherwise force a failure
		 */
		return (ret == TC_SKIP) ? TC_PASS : TC_FAIL;
	}
	/* No expectation was made, no change is needed. */
	return ret;
}

/**
 * @brief Get a friendly name string for a given test phrase.
 *
 * @param phase an enum ztest_phase value describing the desired test phase
 * @returns a string name for `phase`
 */
static inline const char *get_friendly_phase_name(enum ztest_phase phase)
{
	switch (phase) {
	case TEST_PHASE_SETUP:
		return "setup";
	case TEST_PHASE_BEFORE:
		return "before";
	case TEST_PHASE_TEST:
		return "test";
	case TEST_PHASE_AFTER:
		return "after";
	case TEST_PHASE_TEARDOWN:
		return "teardown";
	case TEST_PHASE_FRAMEWORK:
		return "framework";
	default:
		return "(unknown)";
	}
}

static bool current_test_failed_assumption;
void ztest_skip_failed_assumption(void)
{
	if (IS_ENABLED(CONFIG_ZTEST_FAIL_ON_ASSUME)) {
		current_test_failed_assumption = true;
	}
	ztest_test_skip();
}

#ifndef KERNEL

/* Static code analysis tool can raise a violation that the standard header
 * <setjmp.h> shall not be used.
 *
 * setjmp is using in a test code, not in a runtime code, it is acceptable.
 * It is a deliberate deviation.
 */
#include <setjmp.h> /* parasoft-suppress MISRAC2012-RULE_21_4-a MISRAC2012-RULE_21_4-b*/
#include <signal.h>
#include <stdlib.h>
#include <string.h>

#define FAIL_FAST 0

static jmp_buf test_fail;
static jmp_buf test_pass;
static jmp_buf test_skip;
static jmp_buf stack_fail;
static jmp_buf test_suite_fail;

void ztest_test_fail(void)
{
	switch (cur_phase) {
	case TEST_PHASE_SETUP:
		PRINT_DATA(" at %s function\n", get_friendly_phase_name(cur_phase));
		longjmp(test_suite_fail, 1);
	case TEST_PHASE_BEFORE:
	case TEST_PHASE_TEST:
		PRINT_DATA(" at %s function\n", get_friendly_phase_name(cur_phase));
		longjmp(test_fail, 1);
	case TEST_PHASE_AFTER:
	case TEST_PHASE_TEARDOWN:
	case TEST_PHASE_FRAMEWORK:
		PRINT_DATA(" ERROR: cannot fail in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		longjmp(stack_fail, 1);
	}
}
EXPORT_SYMBOL(ztest_test_fail);

void ztest_test_pass(void)
{
	if (cur_phase == TEST_PHASE_TEST) {
		longjmp(test_pass, 1);
	}
	PRINT_DATA(" ERROR: cannot pass in test phase '%s()', bailing\n",
		   get_friendly_phase_name(cur_phase));
	longjmp(stack_fail, 1);
}
EXPORT_SYMBOL(ztest_test_pass);

void ztest_test_skip(void)
{
	switch (cur_phase) {
	case TEST_PHASE_SETUP:
	case TEST_PHASE_BEFORE:
	case TEST_PHASE_TEST:
		longjmp(test_skip, 1);
	default:
		PRINT_DATA(" ERROR: cannot skip in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		longjmp(stack_fail, 1);
	}
}
EXPORT_SYMBOL(ztest_test_skip);

void ztest_test_expect_fail(void)
{
	failed_expectation = true;

	switch (cur_phase) {
	case TEST_PHASE_SETUP:
		PRINT_DATA(" at %s function\n", get_friendly_phase_name(cur_phase));
		break;
	case TEST_PHASE_BEFORE:
	case TEST_PHASE_TEST:
		PRINT_DATA(" at %s function\n", get_friendly_phase_name(cur_phase));
		break;
	case TEST_PHASE_AFTER:
	case TEST_PHASE_TEARDOWN:
	case TEST_PHASE_FRAMEWORK:
		PRINT_DATA(" ERROR: cannot fail in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		longjmp(stack_fail, 1);
	}
}

static int run_test(struct ztest_suite_node *suite, struct ztest_unit_test *test, void *data)
{
	int ret = TC_PASS;

	TC_START(test->name);
	__ztest_set_test_phase(TEST_PHASE_BEFORE);

	if (test_result == ZTEST_RESULT_SUITE_FAIL) {
		ret = TC_FAIL;
		goto out;
	}

	if (setjmp(test_fail)) {
		ret = TC_FAIL;
		goto out;
	}

	if (setjmp(test_pass)) {
		ret = TC_PASS;
		goto out;
	}

	if (setjmp(test_skip)) {
		ret = TC_SKIP;
		goto out;
	}

	run_test_rules(/*is_before=*/true, test, data);
	if (suite->before) {
		suite->before(data);
	}
	run_test_functions(suite, test, data);
out:
	if (failed_expectation) {
		failed_expectation = false;
		ret = TC_FAIL;
	}

	__ztest_set_test_phase(TEST_PHASE_AFTER);
	if (test_result != ZTEST_RESULT_SUITE_FAIL) {
		if (suite->after != NULL) {
			suite->after(data);
		}
		run_test_rules(/*is_before=*/false, test, data);
	}
	__ztest_set_test_phase(TEST_PHASE_FRAMEWORK);
	ret |= cleanup_test(test);

	ret = get_final_test_result(test, ret);
	Z_TC_END_RESULT(ret, test->name);
	if (ret == TC_SKIP && current_test_failed_assumption) {
		test_status = 1;
	}

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

K_THREAD_STACK_DEFINE(ztest_thread_stack, CONFIG_ZTEST_STACK_SIZE + CONFIG_TEST_EXTRA_STACK_SIZE);

static void test_finalize(void)
{
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		k_thread_abort(&ztest_thread);
		if (k_is_in_isr()) {
			return;
		}

		k_thread_abort(k_current_get());
		CODE_UNREACHABLE;
	}
}

void ztest_test_fail(void)
{
	switch (cur_phase) {
	case TEST_PHASE_SETUP:
		__ztest_set_test_result(ZTEST_RESULT_SUITE_FAIL);
		break;
	case TEST_PHASE_BEFORE:
	case TEST_PHASE_TEST:
		__ztest_set_test_result(ZTEST_RESULT_FAIL);
		test_finalize();
		break;
	default:
		PRINT_DATA(" ERROR: cannot fail in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		test_status = ZTEST_STATUS_CRITICAL_ERROR;
		break;
	}
}
EXPORT_SYMBOL(ztest_test_fail);

void ztest_test_pass(void)
{
	switch (cur_phase) {
	case TEST_PHASE_TEST:
		__ztest_set_test_result(ZTEST_RESULT_PASS);
		test_finalize();
		break;
	default:
		PRINT_DATA(" ERROR: cannot pass in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		test_status = ZTEST_STATUS_CRITICAL_ERROR;
		if (cur_phase == TEST_PHASE_BEFORE) {
			test_finalize();
		}
	}
}
EXPORT_SYMBOL(ztest_test_pass);

void ztest_test_skip(void)
{
	switch (cur_phase) {
	case TEST_PHASE_SETUP:
		__ztest_set_test_result(ZTEST_RESULT_SUITE_SKIP);
		break;
	case TEST_PHASE_BEFORE:
	case TEST_PHASE_TEST:
		__ztest_set_test_result(ZTEST_RESULT_SKIP);
		test_finalize();
		break;
	default:
		PRINT_DATA(" ERROR: cannot skip in test phase '%s()', bailing\n",
			   get_friendly_phase_name(cur_phase));
		test_status = ZTEST_STATUS_CRITICAL_ERROR;
		break;
	}
}
EXPORT_SYMBOL(ztest_test_skip);

void ztest_test_expect_fail(void)
{
	failed_expectation = true;
}

void ztest_simple_1cpu_before(void *data)
{
	ARG_UNUSED(data);
	z_test_1cpu_start();
}

void ztest_simple_1cpu_after(void *data)
{
	ARG_UNUSED(data);
	z_test_1cpu_stop();
}

static void test_cb(void *a, void *b, void *c)
{
	struct ztest_suite_node *suite = a;
	struct ztest_unit_test *test = b;
	const bool config_user_mode = FIELD_GET(K_USER, test->thread_options) != 0;

	if (!IS_ENABLED(CONFIG_USERSPACE) || !k_is_user_context()) {
		__ztest_set_test_result(ZTEST_RESULT_PENDING);
		run_test_rules(/*is_before=*/true, test, /*data=*/c);
		if (suite->before) {
			suite->before(/*data=*/c);
		}
		if (IS_ENABLED(CONFIG_USERSPACE) && config_user_mode) {
			k_thread_user_mode_enter(test_cb, a, b, c);
		}
	}
	run_test_functions(suite, test, c);
	__ztest_set_test_result(ZTEST_RESULT_PASS);
}

static int run_test(struct ztest_suite_node *suite, struct ztest_unit_test *test, void *data)
{
	int ret = TC_PASS;

#if CONFIG_ZTEST_TEST_DELAY_MS > 0
	k_busy_wait(CONFIG_ZTEST_TEST_DELAY_MS * USEC_PER_MSEC);
#endif
	TC_START(test->name);

	__ztest_set_test_phase(TEST_PHASE_BEFORE);

	/* If the suite's setup function marked us as skipped, don't bother
	 * running the tests.
	 */
	if (IS_ENABLED(CONFIG_MULTITHREADING)) {
		get_start_time_cyc();
		k_thread_create(&ztest_thread, ztest_thread_stack,
				K_THREAD_STACK_SIZEOF(ztest_thread_stack), test_cb, suite, test,
				data, CONFIG_ZTEST_THREAD_PRIORITY, K_INHERIT_PERMS, K_FOREVER);

		k_thread_access_grant(&ztest_thread, suite, test, suite->stats);
		if (test->name != NULL) {
			k_thread_name_set(&ztest_thread, test->name);
		}
		/* Only start the thread if we're not skipping the suite */
		if (test_result != ZTEST_RESULT_SUITE_SKIP &&
		    test_result != ZTEST_RESULT_SUITE_FAIL) {
			k_thread_start(&ztest_thread);
			k_thread_join(&ztest_thread, K_FOREVER);
		}
	} else if (test_result != ZTEST_RESULT_SUITE_SKIP &&
		   test_result != ZTEST_RESULT_SUITE_FAIL) {
		__ztest_set_test_result(ZTEST_RESULT_PENDING);
		get_start_time_cyc();
		run_test_rules(/*is_before=*/true, test, data);
		if (suite->before) {
			suite->before(data);
		}
		run_test_functions(suite, test, data);
	}

	__ztest_set_test_phase(TEST_PHASE_AFTER);
	if (suite->after != NULL) {
		suite->after(data);
	}
	run_test_rules(/*is_before=*/false, test, data);

	get_test_duration_ms();
	if (tc_spend_time > test->stats->duration_worst_ms) {
		test->stats->duration_worst_ms = tc_spend_time;
	}

	__ztest_set_test_phase(TEST_PHASE_FRAMEWORK);

	/* Flush all logs in case deferred mode and default logging thread are used. */
	while (IS_ENABLED(CONFIG_TEST_LOGGING_FLUSH_AFTER_TEST) &&
	       IS_ENABLED(CONFIG_LOG_PROCESS_THREAD) && log_data_pending()) {
		k_msleep(100);
	}

	if (test_result == ZTEST_RESULT_FAIL || test_result == ZTEST_RESULT_SUITE_FAIL ||
	    failed_expectation) {
		ret = TC_FAIL;
		failed_expectation = false;
	} else if (test_result == ZTEST_RESULT_SKIP || test_result == ZTEST_RESULT_SUITE_SKIP) {
		ret = TC_SKIP;
	}

	if (test_result == ZTEST_RESULT_PASS || !FAIL_FAST) {
		ret |= cleanup_test(test);
	}

	ret = get_final_test_result(test, ret);
	Z_TC_END_RESULT(ret, test->name);
	if (ret == TC_SKIP && current_test_failed_assumption) {
		test_status = 1;
	}

	return ret;
}

#endif /* !KERNEL */

static struct ztest_suite_node *ztest_find_test_suite(const char *name)
{
	struct ztest_suite_node *node;

	for (node = _ztest_suite_node_list_start; node < _ztest_suite_node_list_end; ++node) {
		if (strcmp(name, node->name) == 0) {
			return node;
		}
	}

	return NULL;
}

struct ztest_unit_test *z_ztest_get_next_test(const char *suite, struct ztest_unit_test *prev)
{
	struct ztest_unit_test *test = (prev == NULL) ? _ztest_unit_test_list_start : prev + 1;

	for (; test < _ztest_unit_test_list_end; ++test) {
		if (strcmp(suite, test->test_suite_name) == 0) {
			return test;
		}
	}
	return NULL;
}

#if CONFIG_ZTEST_SHUFFLE
static void z_ztest_shuffle(bool shuffle, void *dest[], intptr_t start, size_t num_items,
			    size_t element_size)
{
	/* Initialize dest array */
	for (size_t i = 0; i < num_items; ++i) {
		dest[i] = (void *)(start + (i * element_size));
	}
	void *tmp;

	/* Shuffle dest array */
	if (shuffle) {
		for (size_t i = num_items - 1; i > 0; i--) {
			int j = sys_rand32_get() % (i + 1);

			if (i != j) {
				tmp = dest[j];
				dest[j] = dest[i];
				dest[i] = tmp;
			}
		}
	}
}
#endif

static int z_ztest_run_test_suite_ptr(struct ztest_suite_node *suite, bool shuffle, int suite_iter,
				      int case_iter, void *param)
{
	struct ztest_unit_test *test = NULL;
	void *data = NULL;
	int fail = 0;
	int tc_result = TC_PASS;

	if (FAIL_FAST && test_status != ZTEST_STATUS_OK) {
		return test_status;
	}

	if (suite == NULL) {
		test_status = ZTEST_STATUS_CRITICAL_ERROR;
		return -1;
	}

#ifndef KERNEL
	if (setjmp(stack_fail)) {
		PRINT_DATA("TESTSUITE crashed.\n");
		test_status = ZTEST_STATUS_CRITICAL_ERROR;
		end_report();
		exit(1);
	}
#else
	k_object_access_all_grant(&ztest_thread);
#endif

	TC_SUITE_START(suite->name);
	current_test_failed_assumption = false;
	__ztest_set_test_result(ZTEST_RESULT_PENDING);
	__ztest_set_test_phase(TEST_PHASE_SETUP);
#ifndef KERNEL
	if (setjmp(test_suite_fail)) {
		__ztest_set_test_result(ZTEST_RESULT_SUITE_FAIL);
	}
#endif
	if (test_result != ZTEST_RESULT_SUITE_FAIL && suite->setup != NULL) {
		data = suite->setup();
	}
	if (param != NULL) {
		data = param;
	}

	for (int i = 0; i < case_iter; i++) {
#ifdef CONFIG_ZTEST_SHUFFLE
		struct ztest_unit_test *tests_to_run[ZTEST_TEST_COUNT];

		memset(tests_to_run, 0, ZTEST_TEST_COUNT * sizeof(struct ztest_unit_test *));
		z_ztest_shuffle(shuffle, (void **)tests_to_run,
				(intptr_t)_ztest_unit_test_list_start, ZTEST_TEST_COUNT,
				sizeof(struct ztest_unit_test));
		for (size_t j = 0; j < ZTEST_TEST_COUNT; ++j) {
			test = tests_to_run[j];
			/* Make sure that the test belongs to this suite */
			if (strcmp(suite->name, test->test_suite_name) != 0) {
				continue;
			}
			if (ztest_api.should_test_run(suite->name, test->name)) {
				test->stats->run_count++;
				tc_result = run_test(suite, test, data);
				if (tc_result == TC_PASS) {
					test->stats->pass_count++;
				} else if (tc_result == TC_SKIP) {
					test->stats->skip_count++;
				} else if (tc_result == TC_FAIL) {
					test->stats->fail_count++;
				}
				if (tc_result == TC_FAIL) {
					fail++;
				}
			}

			if ((fail && FAIL_FAST) || test_status == ZTEST_STATUS_CRITICAL_ERROR) {
				break;
			}
		}
#else
		while (((test = z_ztest_get_next_test(suite->name, test)) != NULL)) {
			if (ztest_api.should_test_run(suite->name, test->name)) {
				test->stats->run_count++;
				tc_result = run_test(suite, test, data);
				if (tc_result == TC_PASS) {
					test->stats->pass_count++;
				} else if (tc_result == TC_SKIP) {
					test->stats->skip_count++;
				} else if (tc_result == TC_FAIL) {
					test->stats->fail_count++;
				}

				if (tc_result == TC_FAIL) {
					fail++;
				}
			}

			if ((fail && FAIL_FAST) || test_status == ZTEST_STATUS_CRITICAL_ERROR) {
				break;
			}
		}
#endif
		if (test_status == ZTEST_STATUS_OK && fail != 0) {
			test_status = ZTEST_STATUS_HAS_FAILURE;
		}
	}

	TC_SUITE_END(suite->name, (fail > 0 ? TC_FAIL : TC_PASS));
	__ztest_set_test_phase(TEST_PHASE_TEARDOWN);
	if (suite->teardown != NULL) {
		suite->teardown(data);
	}

	return fail;
}

int z_ztest_run_test_suite(const char *name, bool shuffle,
	int suite_iter, int case_iter, void *param)
{
	return z_ztest_run_test_suite_ptr(ztest_find_test_suite(name), shuffle, suite_iter,
					  case_iter, param);
}

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(ztest_mem_partition);
#endif

static void __ztest_init_unit_test_result_for_suite(struct ztest_suite_node *suite)
{
	struct ztest_unit_test *test = NULL;

	while (((test = z_ztest_get_next_test(suite->name, test)) != NULL)) {
		test->stats->run_count = 0;
		test->stats->skip_count = 0;
		test->stats->fail_count = 0;
		test->stats->pass_count = 0;
		test->stats->duration_worst_ms = 0;
	}
}

static void flush_log(void)
{
	if (IS_ENABLED(CONFIG_LOG_PROCESS_THREAD)) {
		while (log_data_pending()) {
			k_sleep(K_MSEC(10));
		}
		k_sleep(K_MSEC(10));
	} else {
		while (LOG_PROCESS()) {
		}
	}
}

/* Show one line summary for a test suite.
 */
static void __ztest_show_suite_summary_oneline(struct ztest_suite_node *suite)
{
	int distinct_pass = 0, distinct_fail = 0, distinct_skip = 0, distinct_total = 0;
	int effective_total = 0;
	int expanded_pass = 0, expanded_passrate = 0;
	int passrate_major = 0, passrate_minor = 0, passrate_tail = 0;
	int suite_result = TC_PASS;

	struct ztest_unit_test *test = NULL;
	unsigned int suite_duration_worst_ms = 0;

	/** summary of distinct run  */
	while (((test = z_ztest_get_next_test(suite->name, test)) != NULL)) {
		distinct_total++;
		suite_duration_worst_ms += test->stats->duration_worst_ms;
		if (test->stats->skip_count == test->stats->run_count) {
			distinct_skip++;
		} else if (test->stats->pass_count == test->stats->run_count) {
			distinct_pass++;
		} else {
			distinct_fail++;
		}
	}

	if (distinct_skip == distinct_total) {
		suite_result = TC_SKIP;
		passrate_major = passrate_minor = 0;
	} else {
		suite_result = (distinct_fail > 0) ? TC_FAIL : TC_PASS;
		effective_total = distinct_total - distinct_skip;
		expanded_pass = distinct_pass * 100000;
		expanded_passrate = expanded_pass / effective_total;
		passrate_major = expanded_passrate / 1000;
		passrate_minor = (expanded_passrate - passrate_major * 1000) / 10;
		passrate_tail = expanded_passrate - passrate_major * 1000 - passrate_minor * 10;
		if (passrate_tail >= 5) { /* rounding */
			passrate_minor++;
		}
	}

	TC_SUMMARY_PRINT("SUITE %s - %3d.%02d%% [%s]: pass = %d, fail = %d, "
			 "skip = %d, total = %d duration = %u.%03u seconds\n",
			 TC_RESULT_TO_STR(suite_result), passrate_major, passrate_minor,
			 suite->name, distinct_pass, distinct_fail, distinct_skip, distinct_total,
			 suite_duration_worst_ms / 1000, suite_duration_worst_ms % 1000);
	flush_log();
}

static void __ztest_show_suite_summary_verbose(struct ztest_suite_node *suite)
{
	struct ztest_unit_test *test = NULL;
	int tc_result = TC_PASS;
	int flush_frequency = 0;

	if (IS_ENABLED(CONFIG_ZTEST_VERBOSE_SUMMARY) == 0) {
		return;
	}

	while (((test = z_ztest_get_next_test(suite->name, test)) != NULL)) {
		if (test->stats->skip_count == test->stats->run_count) {
			tc_result = TC_SKIP;
		} else if (test->stats->pass_count == test->stats->run_count) {
			tc_result = TC_PASS;
		} else if (test->stats->pass_count == 0) {
			tc_result = TC_FAIL;
		} else {
			tc_result = TC_FLAKY;
		}

		if (tc_result == TC_FLAKY) {
			TC_SUMMARY_PRINT(
				" - %s - [%s.%s] - (Failed %d of %d attempts)"
				" - duration = %u.%03u seconds\n",
				TC_RESULT_TO_STR(tc_result), test->test_suite_name, test->name,
				test->stats->run_count - test->stats->pass_count,
				test->stats->run_count, test->stats->duration_worst_ms / 1000,
				test->stats->duration_worst_ms % 1000);
		} else {
			TC_SUMMARY_PRINT(" - %s - [%s.%s] duration = %u.%03u seconds\n",
					 TC_RESULT_TO_STR(tc_result), test->test_suite_name,
					 test->name, test->stats->duration_worst_ms / 1000,
					 test->stats->duration_worst_ms % 1000);
		}

		if (flush_frequency % 3 == 0) {
			/** Reduce the flush frequency a bit to speed up the output */
			flush_log();
		}
		flush_frequency++;
	}
	TC_SUMMARY_PRINT("\n");
	flush_log();
}

static void __ztest_show_suite_summary(void)
{
	if (IS_ENABLED(CONFIG_ZTEST_SUMMARY) == 0) {
		return;
	}
	/* Flush the log a lot to ensure that no summary content
	 * is dropped if it goes through the logging subsystem.
	 */
	flush_log();
	TC_SUMMARY_PRINT("\n------ TESTSUITE SUMMARY START ------\n\n");
	flush_log();
	for (struct ztest_suite_node *ptr = _ztest_suite_node_list_start;
	     ptr < _ztest_suite_node_list_end; ++ptr) {

		__ztest_show_suite_summary_oneline(ptr);
		__ztest_show_suite_summary_verbose(ptr);
	}
	TC_SUMMARY_PRINT("------ TESTSUITE SUMMARY END ------\n\n");
	flush_log();
}

static int __ztest_run_test_suite(struct ztest_suite_node *ptr, const void *state, bool shuffle,
				  int suite_iter, int case_iter, void *param)
{
	struct ztest_suite_stats *stats = ptr->stats;
	int count = 0;

	for (int i = 0; i < suite_iter; i++) {
		if (ztest_api.should_suite_run(state, ptr)) {
			int fail = z_ztest_run_test_suite_ptr(ptr, shuffle,
							suite_iter, case_iter, param);

			count++;
			stats->run_count++;
			stats->fail_count += (fail != 0) ? 1 : 0;
		} else {
			stats->skip_count++;
		}
	}

	return count;
}

int z_impl_ztest_run_test_suites(const void *state, bool shuffle, int suite_iter, int case_iter)
{
	int count = 0;
	void *param = NULL;
	if (test_status == ZTEST_STATUS_CRITICAL_ERROR) {
		return count;
	}

#ifdef CONFIG_ZTEST_COVERAGE_RESET_BEFORE_TESTS
	gcov_reset_all_counts();
#endif

#ifdef CONFIG_ZTEST_SHUFFLE
	struct ztest_suite_node *suites_to_run[ZTEST_SUITE_COUNT];

	memset(suites_to_run, 0, ZTEST_SUITE_COUNT * sizeof(struct ztest_suite_node *));
	z_ztest_shuffle(shuffle, (void **)suites_to_run, (intptr_t)_ztest_suite_node_list_start,
			ZTEST_SUITE_COUNT, sizeof(struct ztest_suite_node));
	for (size_t i = 0; i < ZTEST_SUITE_COUNT; ++i) {
		__ztest_init_unit_test_result_for_suite(suites_to_run[i]);
	}
	for (size_t i = 0; i < ZTEST_SUITE_COUNT; ++i) {
		count += __ztest_run_test_suite(suites_to_run[i], state, shuffle, suite_iter,
						case_iter, param);
		/* Stop running tests if we have a critical error or if we have a failure and
		 * FAIL_FAST was set
		 */
		if (test_status == ZTEST_STATUS_CRITICAL_ERROR ||
		    (test_status == ZTEST_STATUS_HAS_FAILURE && FAIL_FAST)) {
			break;
		}
	}
#else
	for (struct ztest_suite_node *ptr = _ztest_suite_node_list_start;
	     ptr < _ztest_suite_node_list_end; ++ptr) {
		__ztest_init_unit_test_result_for_suite(ptr);
		count += __ztest_run_test_suite(ptr, state, shuffle, suite_iter, case_iter, param);
		/* Stop running tests if we have a critical error or if we have a failure and
		 * FAIL_FAST was set
		 */
		if (test_status == ZTEST_STATUS_CRITICAL_ERROR ||
		    (test_status == ZTEST_STATUS_HAS_FAILURE && FAIL_FAST)) {
			break;
		}
	}
#endif

	return count;
}

void z_impl___ztest_set_test_result(enum ztest_result new_result)
{
	test_result = new_result;
}

void z_impl___ztest_set_test_phase(enum ztest_phase new_phase)
{
	cur_phase = new_phase;
}

#ifdef CONFIG_USERSPACE
void z_vrfy___ztest_set_test_result(enum ztest_result new_result)
{
	z_impl___ztest_set_test_result(new_result);
}
#include <zephyr/syscalls/__ztest_set_test_result_mrsh.c>

void z_vrfy___ztest_set_test_phase(enum ztest_phase new_phase)
{
	z_impl___ztest_set_test_phase(new_phase);
}
#include <zephyr/syscalls/__ztest_set_test_phase_mrsh.c>
#endif /* CONFIG_USERSPACE */

void ztest_verify_all_test_suites_ran(void)
{
	bool all_tests_run = true;
	struct ztest_suite_node *suite;
	struct ztest_unit_test *test;

	if (IS_ENABLED(CONFIG_ZTEST_VERIFY_RUN_ALL)) {
		for (suite = _ztest_suite_node_list_start; suite < _ztest_suite_node_list_end;
		     ++suite) {
			if (suite->stats->run_count < 1) {
				PRINT_DATA("ERROR: Test suite '%s' did not run.\n", suite->name);
				all_tests_run = false;
			}
		}

		for (test = _ztest_unit_test_list_start; test < _ztest_unit_test_list_end; ++test) {
			suite = ztest_find_test_suite(test->test_suite_name);
			if (suite == NULL) {
				PRINT_DATA("ERROR: Test '%s' assigned to test suite '%s' which "
					   "doesn't "
					   "exist\n",
					   test->name, test->test_suite_name);
				all_tests_run = false;
			}
		}

		if (!all_tests_run) {
			test_status = ZTEST_STATUS_HAS_FAILURE;
		}
	}

	for (test = _ztest_unit_test_list_start; test < _ztest_unit_test_list_end; ++test) {
		if (test->stats->fail_count + test->stats->pass_count + test->stats->skip_count !=
		    test->stats->run_count) {
			PRINT_DATA("Bad stats for %s.%s\n", test->test_suite_name, test->name);
			test_status = 1;
		}
	}
}

void ztest_run_all(const void *state, bool shuffle, int suite_iter, int case_iter)
{
	ztest_api.run_all(state, shuffle, suite_iter, case_iter);
}

void __weak test_main(void)
{
#if CONFIG_ZTEST_SHUFFLE
	ztest_run_all(NULL, true, NUM_ITER_PER_SUITE, NUM_ITER_PER_TEST);
#else
	ztest_run_all(NULL, false, NUM_ITER_PER_SUITE, NUM_ITER_PER_TEST);
#endif
	ztest_verify_all_test_suites_ran();
}

#ifndef KERNEL
int main(void)
{
	z_init_mock();
	test_main();
	end_report();
#ifdef CONFIG_ZTEST_NO_YIELD
	/*
	 * Rather than yielding to idle thread, keep the part awake so debugger can
	 * still access it, since some SOCs cannot be debugged in low power states.
	 */
	uint32_t key = irq_lock();

	while (1) {
		; /* Spin */
	}
	irq_unlock(key);
#endif
	return test_status;
}
#else

/* Shell */

#ifdef CONFIG_ZTEST_SHELL
static int cmd_list_suites(const struct shell *sh, size_t argc, char **argv)
{
	struct ztest_suite_node *suite;

	for (suite = _ztest_suite_node_list_start; suite < _ztest_suite_node_list_end; ++suite) {
		shell_print(sh, "%s", suite->name);
	}
	return 0;
}

static int cmd_list_cases(const struct shell *sh, size_t argc, char **argv)
{
	struct ztest_suite_node *ptr;
	struct ztest_unit_test *test = NULL;
	int test_count = 0;

	for (ptr = _ztest_suite_node_list_start; ptr < _ztest_suite_node_list_end; ++ptr) {
		test = NULL;
		while ((test = z_ztest_get_next_test(ptr->name, test)) != NULL) {
			shell_print(sh, "%s::%s", test->test_suite_name, test->name);
			test_count++;
		}
	}
	return 0;
}
extern void ztest_set_test_args(char *argv);
extern void ztest_reset_test_args(void);

static int cmd_runall(const struct shell *sh, size_t argc, char **argv)
{
	ztest_reset_test_args();
	ztest_run_all(NULL, false, 1, 1);
	end_report();
	return 0;
}

#ifdef CONFIG_ZTEST_SHUFFLE
static int cmd_shuffle(const struct shell *sh, size_t argc, char **argv)
{

	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"suite_iter", required_argument, 0, 's'},
					       {"case_iter", required_argument, 0, 'c'},
					       {0, 0, 0, 0}};
	int opt_index = 0;
	int val;
	int opt_num = 0;

	int suite_iter = 1;
	int case_iter = 1;

	while ((opt = getopt_long(argc, argv, "s:c:", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 's':
			val = atoi(state->optarg);
			if (val < 1) {
				shell_error(sh, "Invalid number of suite iterations");
				return -ENOEXEC;
			}
			suite_iter = val;
			opt_num++;
			break;
		case 'c':
			val = atoi(state->optarg);
			if (val < 1) {
				shell_error(sh, "Invalid number of case iterations");
				return -ENOEXEC;
			}
			case_iter = val;
			opt_num++;
			break;
		default:
			shell_error(sh, "Invalid option or option usage: %s",
				    argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}
	ztest_reset_test_args();
	ztest_run_all(NULL, true, suite_iter, case_iter);
	end_report();
	return 0;
}
#endif

static int cmd_run_suite(const struct shell *sh, size_t argc, char **argv)
{
	struct getopt_state *state;
	int opt;
	static struct option long_options[] = {{"repeat_iter", required_argument, NULL, 'r'},
		{NULL, 0, NULL, 0}};
	int opt_index = 0;
	int val;
	int opt_num = 0;
	void *param = NULL;
	int repeat_iter = 1;

	while ((opt = getopt_long(argc, argv, "r:p:", long_options, &opt_index)) != -1) {
		state = getopt_state_get();
		switch (opt) {
		case 'r':
			val = atoi(state->optarg);
			if (val < 1) {
				shell_fprintf(sh, SHELL_ERROR,
					"Invalid number of suite interations\n");
				return -ENOEXEC;
			}
			repeat_iter = val;
			opt_num++;
			break;
		case 'p':
			param = state->optarg;
			opt_num++;
			break;
		default:
			shell_fprintf(sh, SHELL_ERROR,
				"Invalid option or option usage: %s\n", argv[opt_index + 1]);
			return -ENOEXEC;
		}
	}
	int count = 0;
	bool shuffle = false;
	const char *shell_command = argv[0];

	/*
	 * This if statement determines which argv contains the test name.
	 * If the optional argument is used, the test name is in the third
	 * argv instead of the first.
	 */
	if (opt_num == 1) {
		ztest_set_test_args(argv[3]);
	} else {
		ztest_set_test_args(argv[1]);
	}

	for (struct ztest_suite_node *ptr = _ztest_suite_node_list_start;
	     ptr < _ztest_suite_node_list_end; ++ptr) {
		__ztest_init_unit_test_result_for_suite(ptr);
		if (strcmp(shell_command, "run-testcase") == 0) {
			count += __ztest_run_test_suite(ptr, NULL, shuffle, 1, repeat_iter, param);
		} else if (strcmp(shell_command, "run-testsuite") == 0) {
			count += __ztest_run_test_suite(ptr, NULL, shuffle, repeat_iter, 1, NULL);
		}
		if (test_status == ZTEST_STATUS_CRITICAL_ERROR ||
		    (test_status == ZTEST_STATUS_HAS_FAILURE && FAIL_FAST)) {
			break;
		}
	}
	return 0;
}

static void testsuite_list_get(size_t idx, struct shell_static_entry *entry);

SHELL_DYNAMIC_CMD_CREATE(testsuite_names, testsuite_list_get);

static size_t testsuite_get_all_static(struct ztest_suite_node const **suites)
{
	*suites = _ztest_suite_node_list_start;
	return _ztest_suite_node_list_end - _ztest_suite_node_list_start;
}

static const struct ztest_suite_node *suite_lookup(size_t idx, const char *prefix)
{
	size_t match_idx = 0;
	const struct ztest_suite_node *suite;
	size_t len = testsuite_get_all_static(&suite);
	const struct ztest_suite_node *suite_end = suite + len;

	while (suite < suite_end) {
		if ((suite->name != NULL) && (strlen(suite->name) != 0) &&
		    ((prefix == NULL) || (strncmp(prefix, suite->name, strlen(prefix)) == 0))) {
			if (match_idx == idx) {
				return suite;
			}
			++match_idx;
		}
		++suite;
	}

	return NULL;
}

static void testsuite_list_get(size_t idx, struct shell_static_entry *entry)
{
	const struct ztest_suite_node *suite = suite_lookup(idx, "");

	entry->syntax = (suite != NULL) ? suite->name : NULL;
	entry->handler = NULL;
	entry->help = NULL;
	entry->subcmd = NULL;
}

/* clang-format off */
	SHELL_STATIC_SUBCMD_SET_CREATE(
		sub_ztest_cmds,
		SHELL_CMD_ARG(run-all, NULL, "Run all tests", cmd_runall, 0, 0),
#ifdef CONFIG_ZTEST_SHUFFLE
		SHELL_COND_CMD_ARG(CONFIG_ZTEST_SHUFFLE, shuffle, NULL,
			"Shuffle tests", cmd_shuffle, 0, 2),
#endif
		SHELL_CMD_ARG(list-testsuites, NULL,
			"List all test suites", cmd_list_suites, 0, 0),
		SHELL_CMD_ARG(list-testcases, NULL,
			"List all test cases", cmd_list_cases, 0, 0),
		SHELL_CMD_ARG(run-testsuite, &testsuite_names,
			"Run test suite", cmd_run_suite, 2, 2),
		SHELL_CMD_ARG(run-testcase, NULL, "Run testcase", cmd_run_suite, 2, 2),
		SHELL_SUBCMD_SET_END /* Array terminated. */
	);
/* clang-format on */

SHELL_CMD_REGISTER(ztest, &sub_ztest_cmds, "Ztest commands", NULL);
#endif /* CONFIG_ZTEST_SHELL */

int main(void)
{
#ifdef CONFIG_USERSPACE
	/* Partition containing globals tagged with ZTEST_DMEM and ZTEST_BMEM
	 * macros. Any variables that user code may reference need to be
	 * placed in this partition if no other memory domain configuration
	 * is made.
	 */
	k_mem_domain_add_partition(&k_mem_domain_default, &ztest_mem_partition);
#ifdef Z_MALLOC_PARTITION_EXISTS
	/* Allow access to malloc() memory */
	k_mem_domain_add_partition(&k_mem_domain_default, &z_malloc_partition);
#endif
#endif /* CONFIG_USERSPACE */

	z_init_mock();
#ifndef CONFIG_ZTEST_SHELL
	test_main();
	end_report();
	flush_log();
	LOG_PANIC();
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
			PRINT_DATA("Reset board #%u to test again\n", state.boots);
			k_msleep(10);
			sys_reboot(SYS_REBOOT_COLD);
		} else {
			PRINT_DATA("Failed after %u attempts\n", state.boots);
			state.boots = 0;
		}
	}
#ifdef CONFIG_ZTEST_NO_YIELD
	/*
	 * Rather than yielding to idle thread, keep the part awake so debugger can
	 * still access it, since some SOCs cannot be debugged in low power states.
	 */
	uint32_t key = irq_lock();

	while (1) {
		; /* Spin */
	}
	irq_unlock(key);
#endif /* CONFIG_ZTEST_NO_YIELD */
#endif /* CONFIG_ZTEST_SHELL */
	return 0;
}
#endif
