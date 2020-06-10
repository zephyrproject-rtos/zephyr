/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <ztest.h>

/**
 * @defgroup subsys_tracing_tests subsys tracing
 * @ingroup all_tests
 * @{
 * @}
 */

/* size of stack area used by each thread */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACKSIZE)
#define LOOP_TIMES 200

#define dummy_test(_name) \
	static void _name(void) \
	{ \
		ztest_test_skip(); \
	}

static struct k_thread thread;
K_THREAD_STACK_DEFINE(thread_stack, STACK_SIZE);

/* define semaphores */
K_SEM_DEFINE(thread1_sem, 1, 1);
K_SEM_DEFINE(thread2_sem, 0, 1);

bool trace_thread_invoked;
bool trace_ISR_invoked;
bool trace_thread_idle_invoked;

/* thread handle for switch */
void thread_handle(void *p1, void *self_sem, void *other_sem)
{
	for (int i = 0; i < LOOP_TIMES; i++) {
		/* take my semaphore */
		k_sem_take(self_sem, K_FOREVER);

		/* wait for a while, then let other thread have a turn. */
		k_sleep(K_MSEC(100));

		k_sem_give(other_sem);
	}
}

/* spawn a thread to generate more tracing data about thread switch. */
void create_handle_thread(void)
{
	k_thread_create(&thread, thread_stack, STACK_SIZE,
		thread_handle, NULL, &thread2_sem, &thread1_sem,
		K_PRIO_PREEMPT(0), K_INHERIT_PERMS,
		K_NO_WAIT);

	thread_handle(NULL, &thread1_sem, &thread2_sem);
}

/**
 * we need to use external tools to handle tracing data
 * which produced by qemu/posix platform with different
 * backends. and use popen api to execute the tools, so
 * below test cases only avalible on posix arch.
 */
#ifdef CONFIG_ARCH_POSIX
#include <unistd.h>
#include <stdlib.h>
#define MAX_BYTES 1024
char result_buf[MAX_BYTES];

/**
 * @brief Test if tracing support thread, ISR, idle thread operation.
 *
 * @details
 * - The consumer of the tracing data shall be able to identify and show
 * the current phase of operation (thread, ISR, idle thread, etc.)
 * based on the provided trace data.
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_function_posix(void)
{
#if !defined(CONFIG_TRACING_BACKEND_POSIX) || defined(CONFIG_TRACING_CTF)
	ztest_test_skip();
#endif
	FILE * fp;
	bool thread_switched = false, isr_entered = false, idle_entered = false;

	fp = fopen("channel0_0", "r");
	if (fp == NULL) {
		zassert_false(1, "fail to open tracing data file.");
	}

	while (fgets(result_buf, MAX_BYTES, fp) != NULL) {
		/* remove space line */
		if ('\n' == result_buf[strlen(result_buf)-1]) {
			result_buf[strlen(result_buf)-1] = '\0';
		}
		if (strstr(result_buf, "sys_trace_thread_switched") != NULL) {
			thread_switched = true;
		}
		if (strstr(result_buf, "isr_enter") != NULL) {
			isr_entered = true;
		}
		if (strstr(result_buf, "idle") != NULL) {
			idle_entered = true;
		}
	}

	zassert_true(thread_switched, "doesn't find thread_switched.");
	zassert_true(isr_entered, "doesn't find isr_entered.");
	zassert_true(idle_entered, "doesn't find idle_entered.");

	fclose(fp);
}

/**
 * @brief Test if tracing support CTF.
 *
 * @details
 * - The tracing system if support the common tracing format (CTF).
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_function_posix_ctf(void)
{
#ifndef CONFIG_TRACING_CTF
	ztest_test_skip();
#endif
	FILE * fp;

	fp = popen("file channel0_0", "r");
	if (fp == NULL) {
		zassert_false(1, "fail to open tracing data file.");
	}

	/* CTF file type is data, not ASCII text
	 * so we check if the file type is data.
	 */
	while (fgets(result_buf, MAX_BYTES, fp) != NULL) {
		/* remove space line */
		if ('\n' == result_buf[strlen(result_buf)-1]) {
			result_buf[strlen(result_buf)-1] = '\0';
		}
		zassert_true(strcmp(result_buf, "channel0_0: data") == 0,
				"CTF should be data format.");
	}

	pclose(fp);
}

/**
 * @brief Test if tracing product redundant code when tracing disabled.
 *
 * @details
 * - All tracing calls shall not be producing code when
 * the tracing system is disable.
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_function_disabled(void)
{
#ifdef CONFIG_TRACING
	ztest_test_skip();
#endif
	FILE * fp;
	/* variables shows whether have related symbols in image. */
	bool thread_clear = false, isr_clrear = false, idle_clear = false;

	/* using objdump to dump zephyr.elf and check if have tracing symbols */
	fp = popen("objdump -t zephyr/zephyr.elf", "r");
	if (fp == NULL) {
		zassert_false(true, "fail to run dump.");
		return;
	}

	while (fgets(result_buf, MAX_BYTES, fp) != NULL) {
		/* remove space line */
		if ('\n' == result_buf[strlen(result_buf)-1]) {
			result_buf[strlen(result_buf)-1] = '\0';
		}
		/* set to true when didon't find any tracing symbol
		 * in zehpyr image.
		 */
		if (strstr(result_buf, "sys_trace_thread_switched") == NULL) {
			thread_clear = true;
		}
		if (strstr(result_buf, "sys_trace_isr") == NULL) {
			isr_clrear = true;
		}
		if (strstr(result_buf, "sys_trace_idle") == NULL) {
			idle_clear = true;
		}
	}

	/* should no tracing symbols in zephyr image
	 * when tracing function disabled.
	 */
	zassert_true(thread_clear, "shouldn't have thread tracing code.");
	zassert_true(isr_clrear, "shouldn't have ISR tracing code.");
	zassert_true(idle_clear, "shouldn't have thread idle tracing code.");

	pclose(fp);
}
#else
/* Skip this case for non-posix platform. */
dummy_test(test_tracing_function_posix);
dummy_test(test_tracing_function_posix_ctf);
dummy_test(test_tracing_function_disabled);
#endif

/**
 * @brief Test if tracing support uart backend.
 *
 * @details
 * - Using qemu platform to imitate uart backend.
 * - and check if the tracing functions are be invoked.
 *
 * @ingroup subsys_tracing_tests
 */
void test_tracing_function_uart(void)
{
#ifndef CONFIG_TRACING_BACKEND_UART
	ztest_test_skip();
#endif

	/* we can't handle trace data in non-native platform,
	 * so use trace functions as a hook to check if those
	 * functions are invoked.
	 */
	zassert_true(trace_thread_invoked,
			"shouldn't have thread tracing code.");
	zassert_true(trace_ISR_invoked,
			"shouldn't have ISR tracing code.");
	zassert_true(trace_thread_idle_invoked,
			"shouldn't have thread idle tracing code.");
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(),
				  &thread, &thread_stack,
				  &thread1_sem, &thread2_sem);

	/* generate more tracing data. */
	create_handle_thread();

	ztest_test_suite(test_tracing,
			 ztest_unit_test(test_tracing_function_posix),
			 ztest_unit_test(test_tracing_function_posix_ctf),
			 ztest_unit_test(test_tracing_function_uart),
			 ztest_unit_test(test_tracing_function_disabled)
			 );

	ztest_run_test_suite(test_tracing);
}
