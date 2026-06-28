/*
 * Copyright (c) 2023, Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/sys/mem_blocks.h>

K_SEM_DEFINE(test_sem, 0, 1);

static void test_thread_entry(void *, void *, void *);
K_THREAD_DEFINE(test_thread, 512 + CONFIG_TEST_EXTRA_STACK_SIZE,
		test_thread_entry, NULL, NULL, NULL,
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

/**
 * @brief Object core statistics API error-handling tests
 *
 * Verify that the object core statistics API correctly reports unsupported and
 * invalid requests by manipulating the statistics descriptor of a thread object
 * core and exercising objects (semaphores) that do not collect statistics.
 *
 * @addtogroup kernel_obj_core_stats_tests
 * @{
 */

/**
 * @brief Verify k_obj_core_stats_enable() rejects objects without an enable
 * operation.
 *
 * @details
 * Statistics can only be enabled for object cores whose type provides an enable
 * operation. This test confirms the API returns -ENOTSUP both for an object
 * type that never supports statistics (a semaphore) and for a thread object
 * core whose enable operation has been temporarily removed.
 *
 * Test steps:
 * - Call k_obj_core_stats_enable() for a semaphore object core.
 * - Clear the thread type's enable operation and call k_obj_core_stats_enable()
 *   for the thread object core, then restore the operation.
 *
 * Expected result:
 * - Both calls return -ENOTSUP.
 *
 * @see k_obj_core_stats_enable()
 * @verifies ZEP-SRS-35-6
 */
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

/**
 * @brief Verify k_obj_core_stats_disable() rejects objects without a disable
 * operation.
 *
 * @details
 * Statistics can only be disabled for object cores whose type provides a disable
 * operation. This test confirms the API returns -ENOTSUP both for an object type
 * that never supports statistics (a semaphore) and for a thread object core
 * whose disable operation has been temporarily removed.
 *
 * Test steps:
 * - Call k_obj_core_stats_disable() for a semaphore object core.
 * - Clear the thread type's disable operation and call
 *   k_obj_core_stats_disable() for the thread object core, then restore it.
 *
 * Expected result:
 * - Both calls return -ENOTSUP.
 *
 * @see k_obj_core_stats_disable()
 * @verifies ZEP-SRS-35-6
 */
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

/**
 * @brief Verify k_obj_core_stats_reset() rejects objects without a reset
 * operation.
 *
 * @details
 * Statistics can only be reset for object cores whose type provides a reset
 * operation. This test confirms the API returns -ENOTSUP both for an object type
 * that never supports statistics (a semaphore) and for a thread object core
 * whose reset operation has been temporarily removed.
 *
 * Test steps:
 * - Call k_obj_core_stats_reset() for a semaphore object core.
 * - Clear the thread type's reset operation and call k_obj_core_stats_reset()
 *   for the thread object core, then restore it.
 *
 * Expected result:
 * - Both calls return -ENOTSUP.
 *
 * @see k_obj_core_stats_reset()
 * @verifies ZEP-SRS-35-7
 */
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

/**
 * @brief Verify k_obj_core_stats_query() rejects objects without a query
 * operation.
 *
 * @details
 * Statistics can only be queried for object cores whose type provides a query
 * operation. This test confirms the API returns -ENOTSUP both for an object type
 * that never supports statistics (a semaphore) and for a thread object core
 * whose query operation has been temporarily removed.
 *
 * Test steps:
 * - Call k_obj_core_stats_query() for a semaphore object core.
 * - Clear the thread type's query operation and call k_obj_core_stats_query()
 *   for the thread object core, then restore it.
 *
 * Expected result:
 * - Both calls return -ENOTSUP.
 *
 * @see k_obj_core_stats_query()
 * @verifies ZEP-SRS-35-5
 */
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

/**
 * @brief Verify k_obj_core_stats_raw() rejects unsupported and invalid raw
 * statistics requests.
 *
 * @details
 * Raw statistics are only available for object cores that provide a raw
 * operation, a valid buffer length and a backing statistics buffer. This test
 * confirms the API reports -ENOTSUP when raw statistics are unsupported and
 * -EINVAL when the buffer length is wrong or the statistics pointer is NULL.
 *
 * Test steps:
 * - Call k_obj_core_stats_raw() for a semaphore object core.
 * - Clear the thread type's raw operation and call it for the thread object
 *   core, then restore the operation.
 * - Call it for the thread object core with a zero-length buffer.
 * - Clear the thread object core's statistics pointer, call it, then restore.
 *
 * Expected result:
 * - The unsupported cases return -ENOTSUP and the invalid-argument cases return
 *   -EINVAL.
 *
 * @see k_obj_core_stats_raw()
 * @verifies ZEP-SRS-35-5
 */
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

/**
 * @brief Verify deregistering statistics detaches the backing buffer from an
 * object core.
 *
 * @details
 * Deregistering statistics removes an object core's backing statistics buffer so
 * that subsequent raw queries fail. This test confirms deregistration is
 * rejected for an object that has no statistics (a semaphore), succeeds for a
 * thread object core, and that raw queries then fail until statistics are
 * re-registered.
 *
 * Test steps:
 * - Call k_obj_core_stats_deregister() for a semaphore object core.
 * - Deregister statistics for the thread object core.
 * - Attempt a raw query for the deregistered thread object core.
 * - Re-register the thread's statistics buffer.
 *
 * Expected result:
 * - Deregistration returns -ENOTSUP for the semaphore and 0 for the thread, and
 *   the raw query for the deregistered thread returns -EINVAL.
 *
 * @see k_obj_core_stats_deregister(), k_obj_core_stats_register()
 * @verifies ZEP-SRS-35-6
 */
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

/**
 * @brief Verify registering statistics validates its arguments and attaches the
 * backing buffer.
 *
 * @details
 * Registering statistics attaches a caller-supplied buffer as an object core's
 * backing statistics storage. This test confirms registration is rejected for an
 * object with no statistics (a semaphore) and when the buffer size is wrong, and
 * that a valid registration redirects raw queries to the new buffer.
 *
 * Test steps:
 * - Attempt to register statistics for a semaphore object core.
 * - Attempt to register a wrong-sized buffer for the thread object core.
 * - Register a correctly sized buffer and confirm a raw query reads from it.
 * - Restore the thread's original statistics buffer.
 *
 * Expected result:
 * - Registration returns -ENOTSUP for the semaphore, -EINVAL for the wrong size,
 *   and 0 for the valid buffer, after which the raw query reflects that buffer.
 *
 * @see k_obj_core_stats_register(), k_obj_core_stats_raw()
 * @verifies ZEP-SRS-35-6
 */
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

/**
 * @}
 */

ZTEST_SUITE(obj_core_stats_api, NULL, NULL,
	    ztest_simple_1cpu_before, ztest_simple_1cpu_after, NULL);
