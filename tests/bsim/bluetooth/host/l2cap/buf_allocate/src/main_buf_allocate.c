/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include <zephyr/bluetooth/buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/l2cap.h>

#include <host/hci_core.h>
#include <host/conn_internal.h>
#include <host/l2cap_internal.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(buf_allocate, LOG_LEVEL_DBG);

#define DATA_SIZE 0x10
#define USER_DATA_SIZE 0x10

/* Only one buffer in the pool */
NET_BUF_POOL_DEFINE(buf_pool, 1, BT_L2CAP_SDU_BUF_SIZE(DATA_SIZE), USER_DATA_SIZE, NULL);

static K_SEM_DEFINE(buf_allocate_sem, 0, 1);

enum {
	BUF_ALLOCATE_TEST_PASSED = 1,
	BUF_ALLOCATE_TEST_FAILED = 2,
};

static atomic_t buf_allocate_test_state;

#define BUF_ALLOCATE_TIMEOUT 2

/* This value should be less than the timeout BUF_ALLOCATE_TIMEOUT */
#define SYS_WORKQ_TEST_TIMEOUT 1

/* This value should be greater than twice the timeout BUF_ALLOCATE_TIMEOUT */
#define BT_WORKQ_TEST_NO_BUF_AVAILABLE_TIMEOUT 5

/* This value should be less than the timeout BUF_ALLOCATE_TIMEOUT */
#define BT_WORKQ_TEST_BUF_AVAILABLE_TIMEOUT 1

#define BT_WORKQ_TEST_TIMEOUT_TOLERANCE_MSEC 100
#define BT_WORKQ_TEST_MIN_WAIT_TIME_MSEC \
	((BUF_ALLOCATE_TIMEOUT * MSEC_PER_SEC) - BT_WORKQ_TEST_TIMEOUT_TOLERANCE_MSEC)

static void no_buf_allocate_work_handler(struct k_work *work)
{
	struct net_buf *buf;
	int64_t start_time;
	int64_t delta;

	buf = bt_l2cap_create_pdu(&buf_pool, 0);
	if (buf != NULL) {
		goto failed;
	}

	buf = bt_l2cap_create_pdu_timeout(&buf_pool, 0, K_NO_WAIT);
	if (buf != NULL) {
		goto failed;
	}

	start_time = k_uptime_get();
	buf = bt_l2cap_create_pdu_timeout(&buf_pool, 0, K_SECONDS(BUF_ALLOCATE_TIMEOUT));
	if (buf != NULL) {
		goto failed;
	}

	delta = k_uptime_delta(&start_time);
	LOG_DBG("Wait time %lld", delta);

	if (bt_is_work_thread() && delta < BT_WORKQ_TEST_MIN_WAIT_TIME_MSEC) {
		goto failed;
	}

	buf = bt_conn_create_pdu(&buf_pool, 0);
	if (buf != NULL) {
		goto failed;
	}

	buf = bt_conn_create_pdu_timeout(&buf_pool, 0, K_NO_WAIT);
	if (buf != NULL) {
		goto failed;
	}

	start_time = k_uptime_get();
	buf = bt_conn_create_pdu_timeout(&buf_pool, 0, K_SECONDS(BUF_ALLOCATE_TIMEOUT));
	if (buf != NULL) {
		goto failed;
	}

	delta = k_uptime_delta(&start_time);
	LOG_DBG("Wait time %lld", delta);

	if (bt_is_work_thread() && delta < BT_WORKQ_TEST_MIN_WAIT_TIME_MSEC) {
		goto failed;
	}

	atomic_set(&buf_allocate_test_state, BUF_ALLOCATE_TEST_PASSED);
	k_sem_give(&buf_allocate_sem);
	return;

failed:
	net_buf_drop(&buf);
	atomic_set(&buf_allocate_test_state, BUF_ALLOCATE_TEST_FAILED);
	k_sem_give(&buf_allocate_sem);
}

static void buf_allocate_work_handler(struct k_work *work)
{
	struct net_buf *buf;

	buf = bt_l2cap_create_pdu(&buf_pool, 0);
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	buf = bt_l2cap_create_pdu_timeout(&buf_pool, 0, K_NO_WAIT);
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	buf = bt_l2cap_create_pdu_timeout(&buf_pool, 0, K_SECONDS(BUF_ALLOCATE_TIMEOUT));
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	buf = bt_conn_create_pdu(&buf_pool, 0);
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	buf = bt_conn_create_pdu_timeout(&buf_pool, 0, K_NO_WAIT);
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	buf = bt_conn_create_pdu_timeout(&buf_pool, 0, K_SECONDS(BUF_ALLOCATE_TIMEOUT));
	if (buf == NULL) {
		goto failed;
	}

	net_buf_drop(&buf);

	atomic_set(&buf_allocate_test_state, BUF_ALLOCATE_TEST_PASSED);
	k_sem_give(&buf_allocate_sem);
	return;

failed:
	atomic_set(&buf_allocate_test_state, BUF_ALLOCATE_TEST_FAILED);
	k_sem_give(&buf_allocate_sem);
}

static K_WORK_DEFINE(no_buf_allocate_work, no_buf_allocate_work_handler);
static K_WORK_DEFINE(buf_allocate_work, buf_allocate_work_handler);

static void test_buf_allocate_main(void)
{
	struct net_buf *buf;
	int err;

	LOG_DBG("*L2CAP buffer allocate started*");

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Can't enable Bluetooth (err %d)", err);
		return;
	}
	LOG_DBG("Bluetooth initialized.");

	buf = net_buf_alloc(&buf_pool, K_NO_WAIT);
	if (buf == NULL) {
		TEST_FAIL("Failed to allocate buffer from pool");
		return;
	}

	atomic_clear(&buf_allocate_test_state);
	k_sem_reset(&buf_allocate_sem);
	k_work_submit(&no_buf_allocate_work);

	err = k_sem_take(&buf_allocate_sem, K_SECONDS(SYS_WORKQ_TEST_TIMEOUT));
	if (err != 0) {
		TEST_FAIL("Semaphore take failed (err %d)", err);
		return;
	}

	if (atomic_get(&buf_allocate_test_state) != BUF_ALLOCATE_TEST_PASSED) {
		TEST_FAIL("Buffer allocation test failed");
		return;
	}

	atomic_clear(&buf_allocate_test_state);
	k_sem_reset(&buf_allocate_sem);
	bt_work_submit(&no_buf_allocate_work);

	err = k_sem_take(&buf_allocate_sem, K_SECONDS(BT_WORKQ_TEST_NO_BUF_AVAILABLE_TIMEOUT));
	if (err != 0) {
		TEST_FAIL("Semaphore take failed (err %d)", err);
		return;
	}

	if (atomic_get(&buf_allocate_test_state) != BUF_ALLOCATE_TEST_PASSED) {
		TEST_FAIL("Buffer allocation test failed");
		return;
	}

	net_buf_drop(&buf);

	atomic_clear(&buf_allocate_test_state);
	k_sem_reset(&buf_allocate_sem);
	k_work_submit(&buf_allocate_work);

	err = k_sem_take(&buf_allocate_sem, K_SECONDS(SYS_WORKQ_TEST_TIMEOUT));
	if (err != 0) {
		TEST_FAIL("Semaphore take failed (err %d)", err);
		return;
	}

	if (atomic_get(&buf_allocate_test_state) != BUF_ALLOCATE_TEST_PASSED) {
		TEST_FAIL("Buffer allocation test failed");
		return;
	}

	atomic_clear(&buf_allocate_test_state);
	k_sem_reset(&buf_allocate_sem);
	bt_work_submit(&buf_allocate_work);

	err = k_sem_take(&buf_allocate_sem, K_SECONDS(BT_WORKQ_TEST_BUF_AVAILABLE_TIMEOUT));
	if (err != 0) {
		TEST_FAIL("Semaphore take failed (err %d)", err);
		return;
	}

	if (atomic_get(&buf_allocate_test_state) != BUF_ALLOCATE_TEST_PASSED) {
		TEST_FAIL("Buffer allocation test failed");
		return;
	}

	TEST_PASS("Test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "l2cap_buf_allocate",
		.test_descr = "l2cap_buf_allocate",
		.test_main_f = test_buf_allocate_main,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_main_buf_allocate_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
