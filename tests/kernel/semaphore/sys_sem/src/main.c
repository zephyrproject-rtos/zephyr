/*
 * Copyright (c) 2021 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <ztest.h>
#include <ztest_error_hook.h>

/* Macro declarations */
#define STACK_SIZE (512 + CONFIG_TEST_EXTRA_STACK_SIZE)
#define SEM_INIT_VAL (0U)
#define SEM_MAX_VAL  (3U)
#define TOTAL_MAX (4U)
#define STACK_NUMS 5
#define PRIO 5
#define LOW_PRIO 8
#define HIGH_PRIO 2

static K_THREAD_STACK_ARRAY_DEFINE(multi_stack_give, STACK_NUMS, STACK_SIZE);
static K_THREAD_STACK_ARRAY_DEFINE(multi_stack_take, STACK_NUMS, STACK_SIZE);

static struct k_thread multi_tid_give[STACK_NUMS];
static struct k_thread multi_tid_take[STACK_NUMS];
static struct k_sem usage_sem, sync_sem, limit_sem, uninit_sem;
static ZTEST_DMEM int flag;
static ZTEST_DMEM atomic_t atomic_count;

/**
 * @defgroup kernel_sys_sem_tests Semaphore
 * @ingroup all_tests
 * @{
 * @}
 */

static void sem_thread_give_uninit(void *p1, void *p2, void *p3)
{
	ztest_set_fault_valid(true);

	/* use sem without initialise */
	k_sem_give(&uninit_sem);

	ztest_test_fail();
}

static void sem_thread_give(void *p1, void *p2, void *p3)
{
	flag = 1;
	k_sem_give(&usage_sem);
}

static void thread_low_prio_sem_take(void *p1, void *p2, void *p3)
{
	k_sem_take(&usage_sem, K_FOREVER);

	flag = LOW_PRIO;
	k_sem_give(&sync_sem);
}

static void thread_high_prio_sem_take(void *p1, void *p2, void *p3)
{
	k_sem_take(&usage_sem, K_FOREVER);

	flag = HIGH_PRIO;
	k_sem_give(&sync_sem);
}

/**
 * @brief Test semaphore usage with multiple thread
 *
 * @details Using semaphore with some situations
 * - Use a uninitialized semaphore
 * - Use semaphore normally
 * - Use semaphore with different priority threads
 *
 * @ingroup kernel_sys_sem_tests
 */
void test_multiple_thread_sem_usage(void)
{
	k_sem_init(&usage_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	k_sem_init(&sync_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	/* Use a semaphore to synchronize processing between threads */
	k_sem_reset(&usage_sem);
	k_thread_create(&multi_tid_give[0], multi_stack_give[0], STACK_SIZE,
			sem_thread_give, NULL, NULL,
			NULL, PRIO, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sem_take(&usage_sem, K_FOREVER);
	zassert_equal(flag, 1, "value != 1");
	zassert_equal(k_sem_count_get(&usage_sem), 0, "sem not be took");

	k_sem_reset(&usage_sem);
	/* Use sem with different priority thread */
	k_thread_create(&multi_tid_take[0], multi_stack_take[0], STACK_SIZE,
			thread_low_prio_sem_take, NULL, NULL,
			NULL, LOW_PRIO, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_thread_create(&multi_tid_take[1], multi_stack_take[1], STACK_SIZE,
			thread_high_prio_sem_take, NULL, NULL,
			NULL, HIGH_PRIO, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);

	k_sleep(K_MSEC(50));

	/* Verify if the high prio thread take sem first */
	k_sem_give(&usage_sem);
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(flag, HIGH_PRIO, "high prio value error");

	k_sem_give(&usage_sem);
	k_sem_take(&sync_sem, K_FOREVER);
	zassert_equal(flag, LOW_PRIO, "low prio value error");

	k_thread_join(&multi_tid_give[0], K_FOREVER);
	k_thread_join(&multi_tid_take[0], K_FOREVER);
	k_thread_join(&multi_tid_take[1], K_FOREVER);

	k_thread_create(&multi_tid_give[1], multi_stack_give[1], STACK_SIZE,
			sem_thread_give_uninit, NULL, NULL,
			NULL, PRIO, K_USER | K_INHERIT_PERMS,
			K_NO_WAIT);
	k_sleep(K_MSEC(20));
	k_thread_join(&multi_tid_give[1], K_FOREVER);
}

static void multi_thread_sem_give(void *p1, void *p2, void *p3)
{
	int count;

	(void)atomic_inc(&atomic_count);
	count = atomic_get(&atomic_count);
	k_sem_give(&limit_sem);

	if (count < TOTAL_MAX) {
		zassert_equal(k_sem_count_get(&limit_sem), count, "multi get sem error");
	} else {
		zassert_equal(k_sem_count_get(&limit_sem), SEM_MAX_VAL, "count > SEM_MAX_VAL");
	}

	k_sem_take(&sync_sem, K_FOREVER);
}

static void multi_thread_sem_take(void *p1, void *p2, void *p3)
{
	int count;

	k_sem_take(&limit_sem, K_FOREVER);
	(void)atomic_dec(&atomic_count);
	count = atomic_get(&atomic_count);

	if (count >= 0) {
		zassert_equal(k_sem_count_get(&limit_sem), count, "multi take sem error");
	} else {
		zassert_equal(k_sem_count_get(&limit_sem), 0, "count < SEM_INIT_VAL");
	}

	k_sem_give(&sync_sem);
}

/**
 * @brief Test max semaphore can be give and take with multiple thread
 *
 * @details
 * - Define and initialize semaphore and thread.
 * - Give sem by multiple threads.
 * - Verify more than max count about semaphore can reach.
 * - Take sem by multiple threads and verify if sem count is correct.
 *
 * @ingroup kernel_sys_sem_tests
 */
void test_multi_thread_sem_limit(void)
{
	k_sem_init(&limit_sem, SEM_INIT_VAL, SEM_MAX_VAL);
	k_sem_init(&sync_sem, SEM_INIT_VAL, SEM_MAX_VAL);

	(void)atomic_set(&atomic_count, 0);
	for (int i = 1; i <= TOTAL_MAX; i++) {
		k_thread_create(&multi_tid_give[i], multi_stack_give[i], STACK_SIZE,
				multi_thread_sem_give, NULL, NULL, NULL,
				i, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}

	k_sleep(K_MSEC(50));

	(void)atomic_set(&atomic_count, SEM_MAX_VAL);
	for (int i = 1; i <= TOTAL_MAX; i++) {
		k_thread_create(&multi_tid_take[i], multi_stack_take[i], STACK_SIZE,
				multi_thread_sem_take, NULL, NULL, NULL,
				PRIO, K_USER | K_INHERIT_PERMS, K_NO_WAIT);
	}
}

void test_main(void)
{
	k_thread_access_grant(k_current_get(), &usage_sem, &sync_sem, &limit_sem,
			      &multi_tid_give[0], &multi_tid_give[1],
			      &multi_tid_give[2], &multi_tid_give[3],
			      &multi_tid_give[4], &multi_tid_take[4],
			      &multi_tid_take[2], &multi_tid_take[3],
			      &multi_tid_take[0], &multi_tid_take[1],
			      &multi_tid_give[5], &multi_tid_take[5],
			      &multi_stack_take[0], &multi_stack_take[1],
			      &multi_stack_take[3], &multi_stack_take[4],
			      &multi_stack_take[2], &multi_stack_give[0],
			      &multi_stack_give[1], &multi_stack_give[2],
			      &multi_stack_give[3], &multi_stack_give[4]);

	ztest_test_suite(test_kernel_sys_sem,
			 ztest_1cpu_user_unit_test(test_multiple_thread_sem_usage),
			 ztest_1cpu_user_unit_test(test_multi_thread_sem_limit));
	ztest_run_test_suite(test_kernel_sys_sem);
}
