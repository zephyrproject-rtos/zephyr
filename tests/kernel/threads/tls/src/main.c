/*
 * Copyright (c) 2020 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/app_memory/app_memdomain.h>
#include <zephyr/sys/libc-hooks.h>
#include <zephyr/sys/util.h>

#define NUM_THREADS	3
#define STACK_SIZE	(2048 + CONFIG_TEST_EXTRA_STACK_SIZE)

#define STATIC_DATA8	0x7FU
#define STATIC_DATA32	0xABCDEF00U
#define STATIC_DATA64	0x1122334455667788UL

#define PREFIX_8	0x30U
#define PREFIX_32	0x44668800U
#define PREFIX_64	0xFFEEDDCC00000000UL

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
static uint8_t Z_THREAD_LOCAL thread_data8 = STATIC_DATA8;
static uint32_t Z_THREAD_LOCAL thread_data32 = STATIC_DATA32;
static uint64_t Z_THREAD_LOCAL thread_data64 = STATIC_DATA64;

/* Zeroed thread data */
static uint8_t Z_THREAD_LOCAL thread_bss8;
static uint32_t Z_THREAD_LOCAL thread_bss32;
static uint64_t Z_THREAD_LOCAL thread_bss64;

static void tls_thread_entry(void *p1, void *p2, void *p3)
{
	uint32_t idx;

	idx = (uint32_t)POINTER_TO_UINT(p1);

	/* Check if TLS area in stack is initialized correctly */
	if (thread_data8 != STATIC_DATA8) {
		tls_result[idx] = ERR_BAD_STATIC_DATA;
		goto out;
	}

	if (thread_data32 != STATIC_DATA32) {
		tls_result[idx] = ERR_BAD_STATIC_DATA;
		goto out;
	}

	if (thread_data64 != STATIC_DATA64) {
		tls_result[idx] = ERR_BAD_STATIC_DATA;
		goto out;
	}

	if (thread_bss8 != 0) {
		tls_result[idx] = ERR_BSS_NOT_ZERO;
		goto out;
	}

	if (thread_bss32 != 0) {
		tls_result[idx] = ERR_BSS_NOT_ZERO;
		goto out;
	}

	if (thread_bss64 != 0) {
		tls_result[idx] = ERR_BSS_NOT_ZERO;
		goto out;
	}

	/* Set thread data and see if they remain unchanged */
	thread_data8 = STATIC_DATA8 + idx;
	thread_bss8 = PREFIX_8 + idx;

	thread_data32 = STATIC_DATA32 + idx;
	thread_bss32 = PREFIX_32 + idx;

	thread_data64 = STATIC_DATA64 + idx;
	thread_bss64 = PREFIX_64 + idx;

	/* Let other threads run */
	k_sleep(K_MSEC(100));

	if (thread_data8 != (STATIC_DATA8 + idx)) {
		tls_result[idx] = ERR_DATA_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_data32 != (STATIC_DATA32 + idx)) {
		tls_result[idx] = ERR_DATA_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_data64 != (STATIC_DATA64 + idx)) {
		tls_result[idx] = ERR_DATA_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_bss8 != (PREFIX_8 + idx)) {
		tls_result[idx] = ERR_BSS_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_bss32 != (PREFIX_32 + idx)) {
		tls_result[idx] = ERR_BSS_CHANGED_BY_OTHERS;
		goto out;
	}

	if (thread_bss64 != (PREFIX_64 + idx)) {
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

/**
 * @brief Verify that thread-local variables are private to each thread.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * A variable declared thread-local must resolve to a separate instance per
 * thread, so writes made by one thread are invisible to every other one.
 * Several threads run the same body concurrently, each writing a value derived
 * from its own identity and then checking that what it reads back is still its
 * own; a shared instance would show up as a thread observing a neighbour's
 * value. Skipped when userspace is enabled, as the user-mode case below covers
 * that configuration.
 *
 * Test steps:
 * - Spawn the worker threads in supervisor mode.
 * - In each thread, write to the thread-local variables, yield to let the
 *   others run, and read them back.
 * - Collect the per-thread results once every thread has finished.
 *
 * Expected result:
 * - Every thread reads back only the values it wrote itself.
 *
 * @see Z_THREAD_LOCAL
 */
ZTEST(thread_tls, test_tls_vars_are_per_thread)
{
	if (IS_ENABLED(CONFIG_USERSPACE)) {
		ztest_test_skip();
	}

	/* TLS test in supervisor mode */
	start_tls_test(0);
}

/**
 * @brief Verify that thread-local variables stay private in user mode.
 *
 * @ingroup kernel_thread_tests
 *
 * @details
 * User threads reach their thread-local storage through the same mechanism as
 * supervisor threads but under memory protection, where the TLS block has to
 * be mapped into the thread's own domain. The same isolation check is
 * therefore repeated with the workers spawned as user threads, so a TLS block
 * that is shared or unreachable under userspace is caught.
 *
 * Test steps:
 * - Spawn the worker threads with K_USER, inheriting the caller's permissions.
 * - In each thread, write to the thread-local variables, yield to let the
 *   others run, and read them back.
 * - Collect the per-thread results once every thread has finished.
 *
 * Expected result:
 * - Every user thread reads back only the values it wrote itself.
 *
 * @see Z_THREAD_LOCAL
 * @see k_thread_create()
 */
ZTEST_USER(thread_tls, test_tls_vars_are_per_thread_user)
{
	/* TLS test in user mode */
	start_tls_test(K_USER | K_INHERIT_PERMS);
}

void *thread_tls_setup(void)
{
#ifdef CONFIG_USERSPACE
	int ret;
	unsigned int i;

	struct k_mem_partition *parts[] = {
		&part_common,
#if Z_LIBC_PARTITION_EXISTS
		&z_libc_partition,
#endif
		&ztest_mem_partition,
	};

	parts[0] = &part_common;

	ret = k_mem_domain_init(&dom_common, ARRAY_SIZE(parts), parts);
	__ASSERT(ret == 0, "k_mem_domain_init() failed %d", ret);
	ARG_UNUSED(ret);

	k_mem_domain_add_thread(&dom_common, k_current_get());

	for (i = 0; i < NUM_THREADS; i++) {
		k_thread_access_grant(k_current_get(),
				      &tls_thread[i], &tls_stack[i]);
	}
#endif /* CONFIG_USERSPACE */

	return NULL;
}

ZTEST_SUITE(thread_tls, NULL, thread_tls_setup, NULL, NULL, NULL);
