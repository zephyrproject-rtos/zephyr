/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

static K_SEM_DEFINE(test_sem, 0, 1);
static struct k_timeout_record *test_record;

extern void *common_setup(void);

static void test_before(void *f)
{
	k_sem_reset(&test_sem);
	test_record = NULL;
}

ZTEST_SUITE(timeout_record, NULL, common_setup, test_before, NULL, NULL);

static void test_timeout_handler(struct k_timeout_record *record)
{
	test_record = record;
	k_sem_give(&test_sem);
}

ZTEST(timeout_record, test_timeout_add_elapse_abort)
{
	struct k_timeout_record record;

	k_timeout_record_init(&record);
	(void)k_timeout_record_add(&record, test_timeout_handler, K_NO_WAIT);
	zassert_ok(k_sem_take(&test_sem, K_MSEC(100)));
	zassert_equal(&record, test_record);
	zassert_false(k_timeout_record_abort(&record));
}

ZTEST(timeout_record, test_timeout_add_abort)
{
	struct k_timeout_record record;

	k_timeout_record_init(&record);
	(void)k_timeout_record_add(&record, test_timeout_handler, K_MSEC(1000));
	zassert_equal(k_sem_take(&test_sem, K_MSEC(500)), -EAGAIN);
	zassert_true(k_timeout_record_abort(&record));
	zassert_equal(k_sem_take(&test_sem, K_MSEC(1000)), -EAGAIN);
}
