/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

K_SEM_DEFINE(test_sem, 0, 1);

static void test_thread_entry(void *, void *, void *);
K_THREAD_DEFINE(test_thread, 512, test_thread_entry, NULL, NULL, NULL,
		K_HIGHEST_THREAD_PRIO, 0, 0);

/*
 * As the test mucks about with the set of object core statistics operators
 * for the test_thread, we want to ensure that no other test mucks with it
 * at an inopportune time. This could also  be done by setting the CPU count
 * in the prj.conf to 1. However, for this test to allow it to be run on both
 * UP and SMP systems the mutex is being used.
 */

K_MUTEX_DEFINE(test_mutex);

static void test_thread_entry(void *p1, void *p2, void *p3)
{
	k_sem_take(&test_sem, K_FOREVER);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_enable)
{
	int  status;
	int (*saved_enable)(struct k_obj_core *obj_core);

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to enable stats for an object core that is not enabled
	 * for statistics (semaphores).
	 */

	status = k_obj_core_stats_enable(K_OBJ_CORE(&test_sem));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	saved_enable = K_OBJ_CORE(test_thread)->type->stats_desc->enable;
	K_OBJ_CORE(test_thread)->type->stats_desc->enable = NULL;
	status = k_obj_core_stats_enable(K_OBJ_CORE(test_thread));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);
	K_OBJ_CORE(test_thread)->type->stats_desc->enable = saved_enable;

	/*
	 * Note: Testing the stats enable function pointer is done in another
	 * set of tests.
	 */

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_disable)
{
	int  status;
	int (*saved_disable)(struct k_obj_core *obj_core);

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to disable stats for an object core that is not enabled
	 * for statistics (semaphores).
	 */

	status = k_obj_core_stats_disable(K_OBJ_CORE(&test_sem));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	saved_disable = K_OBJ_CORE(test_thread)->type->stats_desc->disable;
	K_OBJ_CORE(test_thread)->type->stats_desc->disable = NULL;
	status = k_obj_core_stats_disable(K_OBJ_CORE(test_thread));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);
	K_OBJ_CORE(test_thread)->type->stats_desc->disable = saved_disable;

	/*
	 * Note: Testing the stats disable function pointer is done in
	 * another set of tests.
	 */

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_reset)
{
	int  status;
	int (*saved_reset)(struct k_obj_core *obj_core);

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to reset stats for an object core that is not enabled
	 * for statistics (semaphores).
	 */

	status = k_obj_core_stats_reset(K_OBJ_CORE(&test_sem));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	saved_reset = K_OBJ_CORE(test_thread)->type->stats_desc->reset;
	K_OBJ_CORE(test_thread)->type->stats_desc->reset = NULL;
	status = k_obj_core_stats_reset(K_OBJ_CORE(test_thread));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);
	K_OBJ_CORE(test_thread)->type->stats_desc->reset = saved_reset;

	/*
	 * Note: Testing the stats reset function pointer is done in
	 * another set of tests.
	 */

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_query)
{
	int  status;
	struct k_thread_runtime_stats query;
	int (*saved_query)(struct k_obj_core *obj_core, void *stats);

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to query stats for an object core that is not enabled
	 * for statistics (semaphores).
	 */

	status = k_obj_core_stats_query(K_OBJ_CORE(&test_sem), &query,
					sizeof(struct k_thread_runtime_stats));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	saved_query = K_OBJ_CORE(test_thread)->type->stats_desc->query;
	K_OBJ_CORE(test_thread)->type->stats_desc->query = NULL;
	status = k_obj_core_stats_query(K_OBJ_CORE(test_thread),
					&query, sizeof(query));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);
	K_OBJ_CORE(test_thread)->type->stats_desc->query = saved_query;

	/*
	 * Note: Testing the stats query function pointer is done in
	 * another set of tests.
	 */

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_raw)
{
	int  status;
	char buffer[sizeof(struct k_cycle_stats)];
	int (*saved_raw)(struct k_obj_core *obj_core, void *stats);
	void *saved_stats;

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to get raw stats for an object core that is not enabled
	 * for statistics (semaphores).
	 */

	status = k_obj_core_stats_raw(K_OBJ_CORE(&test_sem),
				      buffer, sizeof(buffer));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	/* Force there to be no means to obtain raw data */

	saved_raw = K_OBJ_CORE(test_thread)->type->stats_desc->raw;
	K_OBJ_CORE(test_thread)->type->stats_desc->raw = NULL;
	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread),
				      buffer, sizeof(buffer));
	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n", -ENOTSUP, status);

	K_OBJ_CORE(test_thread)->type->stats_desc->raw = saved_raw;

	/*
	 * Verify that passing a buffer with unexpected length
	 * returns the expected error (-EINVAL).
	 */

	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread),
				      buffer, 0);
	zassert_equal(status, -EINVAL,
		      "Expected %d, got %d\n", -EINVAL, status);

	/*
	 * Verify that if the object core's pointer to raw stats data
	 * is NULL, we get the expected error (-EINVAL).
	 */

	saved_stats = K_OBJ_CORE(test_thread)->stats;
	K_OBJ_CORE(test_thread)->stats = NULL;
	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread),
				      buffer, sizeof(buffer));
	zassert_equal(status, -EINVAL,
		      "Expected %d, got %d\n", -EINVAL, status);
	K_OBJ_CORE(test_thread)->stats = saved_stats;

	/*
	 * Note: Further testing the stats query function pointer is done in
	 * another set of tests.
	 */

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_dereg)
{
	int  status;
	char buffer[sizeof(struct k_cycle_stats)];

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to de-register stats for an object core that does
	 * not have them enabled (semaphores).
	 */

	status = k_obj_core_stats_deregister(K_OBJ_CORE(&test_sem));
	zassert_equal(status, -ENOTSUP, "Expected %d, got %d\n", 0, -ENOTSUP);

	/* De-register stats for the test thread. */

	status = k_obj_core_stats_deregister(K_OBJ_CORE(test_thread));
	zassert_equal(status, 0, "Expected %d, got %d\n", 0, status);

	/* Attempt to get raw stats for the de-registered thread */

	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread),
				      buffer, sizeof(buffer));
	zassert_equal(status, -EINVAL,
		      "Expected %d, got %d\n", -EINVAL, status);

	/* Restore the raw stats */

	status = k_obj_core_stats_register(K_OBJ_CORE(test_thread),
					   &test_thread->base.usage,
					   sizeof(struct k_cycle_stats));

	k_mutex_unlock(&test_mutex);
}

ZTEST(obj_core_stats_api, test_obj_core_stats_register)
{
	int  status;
	char buffer[sizeof(struct k_cycle_stats)];
	char data[sizeof(struct k_cycle_stats)];

	/*
	 * Ensure only one thread is mucking around with test_thread
	 * at a time.
	 */

	k_mutex_lock(&test_mutex, K_FOREVER);

	/*
	 * Attempt to register stats for a semaphore
	 * (which does not currently exist).
	 */

	status = k_obj_core_stats_register(K_OBJ_CORE(&test_sem),
					   (void *)0xBAD0BAD1,
					   42);

	zassert_equal(status, -ENOTSUP,
		      "Expected %d, got %d\n"
		      "--Were semaphore stats recently implemented?\n",
		      -ENOTSUP, status);
	/*
	 * Attempt to register stats for a thread with the wrong buffer
	 * size.
	 */

	status = k_obj_core_stats_register(K_OBJ_CORE(test_thread),
					   buffer, sizeof(buffer) + 42);

	zassert_equal(status, -EINVAL,
		      "Expected %d, got %d\n", -EINVAL, status);

	/*
	 * Attempt to register stats for a thread with the right buffer
	 * size.
	 */

	status = k_obj_core_stats_register(K_OBJ_CORE(test_thread),
					   buffer, sizeof(buffer));

	zassert_equal(status, 0,
		      "Failed to change raw buffer pointer (%d)\n", status);

	memset(buffer, 0xaa, sizeof(buffer));
	memset(data, 0x00, sizeof(data));

	status = k_obj_core_stats_raw(K_OBJ_CORE(test_thread),
				      data, sizeof(data));
	zassert_equal(status, 0, "Expected %d, got %d\n", 0, status);

	zassert_mem_equal(buffer, data, sizeof(buffer),
			  "Test thread raw stats buffer was not changed\n");

	/* Restore the test thread's raw stats buffer */

	status = k_obj_core_stats_register(K_OBJ_CORE(test_thread),
					   &test_thread->base.usage,
					   sizeof(test_thread->base.usage));
	zassert_equal(status, 0,
		      "Expected %d, got %d\n", 0, status);

	k_mutex_unlock(&test_mutex);
}

ZTEST_SUITE(obj_core_stats_api, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
