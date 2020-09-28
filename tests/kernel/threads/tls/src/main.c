/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <kernel.h>
#include <kernel_structs.h>
#include <app_memory/app_memdomain.h>
#include <sys/libc-hooks.h>
#include <sys/util.h>

#define NUM_THREADS	3
#define STACK_SIZE	(512 + CONFIG_TEST_EXTRA_STACKSIZE)

#define STATIC_DATA	0xABCDEF00U

#ifdef CONFIG_USERSPACE
K_APPMEM_PARTITION_DEFINE(part_common);
struct k_mem_domain dom_common;
#endif /* CONFIG_USERSPACE */

enum test_result {
	TEST_OK,

	/* When thread_data* != STATIC_DATA at thread entry */
	ERR_BAD_STATIC_DATA,

	/* When thread_bss* != 0 at thread entry */
	ERR_BSS_NOT_ZERO,

	/* If data/bss is changed by other threads */
	ERR_DATA_CHANGED_BY_OTHERS,
	ERR_BSS_CHANGED_BY_OTHERS,

	TEST_NOT_STARTED,
};

static K_THREAD_STACK_ARRAY_DEFINE(tls_stack, NUM_THREADS, STACK_SIZE);

static struct k_thread tls_thread[NUM_THREADS];

K_APP_BMEM(part_common) static k_tid_t tls_tid[NUM_THREADS];
K_APP_BMEM(part_common) static enum test_result tls_result[NUM_THREADS];

/* Thread data with initialized values */
static uint32_t __thread thread_data01 = STATIC_DATA;

/* Zeroed thread data */
static uint32_t __thread thread_bss01;

static void tls_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t idx;

	idx = (uint32_t)POINTER_TO_UINT(p1);

	/* Check if TLS area in stack is initialized correctly */
	if (thread_data01 != STATIC_DATA) {
		tls_result[idx] = ERR_BAD_STATIC_DATA;
		goto out;
	}

	if (thread_bss01 != 0) {
		tls_result[idx] = ERR_BSS_NOT_ZERO;
		goto out;
	}

	/* Set thread data and see if they remain unchanged */
	thread_data01 = STATIC_DATA + idx;
	thread_bss01 = idx;

	/* Let other threads run */
	k_sleep(K_MSEC(100));

	if (thread_data01 != (STATIC_DATA + idx)) {
		tls_result[idx] = ERR_DATA_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_bss01 != idx) {
		tls_result[idx] = ERR_BSS_CHANGED_BY_OTHERS;
		goto out;
	}

	/* Values are all expected. Test passed */
	tls_result[idx] = TEST_OK;

out:
	return;
}

static void start_tls_test(uint32_t thread_options)
{
	unsigned int i;
	bool passed;

	/* Create threads */
	for (i = 0; i < NUM_THREADS; i++) {
		tls_result[i] = TEST_NOT_STARTED;
		tls_tid[i] = k_thread_create(&tls_thread[i], tls_stack[i],
					     STACK_SIZE, tls_thread_entry,
					     UINT_TO_POINTER(i), NULL, NULL,
					     0, thread_options, K_NO_WAIT);
	}

	/* Wait for all threads to run */
	k_sleep(K_MSEC(500));

	/* Stop all threads */
	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_abort(tls_tid[i]);
		k_thread_join(&tls_thread[i], K_FOREVER);
	}

	/* Check test results */
	passed = true;
	for (i = 0; i < NUM_THREADS; i++) {
		TC_PRINT("thread %d: result %d (expecting %d)\n",
			 i, tls_result[i], TEST_OK);
		if (tls_result[i] != TEST_OK) {
			passed = false;
		}
	}

	zassert_true(passed, "Test failed");
}

void test_tls(void)
{
	/* TLS test in supervisor mode */
	start_tls_test(0);
}

#ifdef CONFIG_USERSPACE
void test_tls_userspace(void)
{
	/* TLS test in supervisor mode */
	start_tls_test(K_USER | K_INHERIT_PERMS);
}
#endif

void test_main(void)
{
#ifdef CONFIG_USERSPACE
	unsigned int i;

	struct k_mem_partition *parts[] = {
		&part_common,
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ztest_mem_partition,
	};

	parts[0] = &part_common;
	k_mem_domain_init(&dom_common, ARRAY_SIZE(parts), parts);
	k_mem_domain_add_thread(&dom_common, k_current_get());

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_access_grant(k_current_get(),
				      &tls_thread[i], &tls_stack[i]);
	}
#endif /* CONFIG_USERSPACE */

	ztest_test_suite(thread_tls,
			 ztest_unit_test(test_tls));
	ztest_run_test_suite(thread_tls);

#ifdef CONFIG_USERSPACE
	ztest_test_suite(thread_tls_userspace,
			 ztest_user_unit_test(test_tls_userspace));
	ztest_run_test_suite(thread_tls_userspace);
#endif /* CONFIG_USERSPACE */
}
