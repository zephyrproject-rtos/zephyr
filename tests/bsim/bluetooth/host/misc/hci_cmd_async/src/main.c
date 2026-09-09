/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>

/* Host-internal API under test */
#include "host/hci_core.h"

#include "babblekit/testcase.h"
#include "bstests.h"

#define WAIT_TIMEOUT K_SECONDS(2)

/* An opcode the controller does not implement */
#define UNKNOWN_OPCODE BT_OP(BT_OGF_LE, 0x3ff)

static K_THREAD_STACK_DEFINE(workq_stack, 1024);
static struct k_work_q workq;

static K_SEM_DEFINE(cb_sem, 0, 1);
static uint8_t cb_status;
static k_tid_t cb_thread;

static void cmd_cb(uint8_t status, struct net_buf *rsp, void *user_data)
{
	TEST_ASSERT(user_data == &cb_sem, "Unexpected user data %p", user_data);
	TEST_ASSERT((rsp != NULL) == (status == 0),
		    "Response %p does not match status 0x%02x", rsp, status);

	cb_status = status;
	cb_thread = k_current_get();

	if (rsp != NULL) {
		net_buf_unref(rsp);
	}

	k_sem_give(&cb_sem);
}

static void random_addr_encode(struct net_buf *buf, void *user_data)
{
	const bt_addr_t *addr = user_data;

	net_buf_add_mem(buf, addr, sizeof(*addr));
}

static void test_future(void)
{
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected status 0x%02x", fut.result);
	TEST_ASSERT(fut.data != NULL, "No response");
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation still pending");

	struct net_buf *rsp = fut.data;

	TEST_ASSERT(rsp->len == sizeof(struct bt_hci_rp_read_bd_addr),
		    "Unexpected response length %u", rsp->len);
	net_buf_unref(rsp);
}

static void test_encoder(void)
{
	bt_addr_t addr = { { 0x11, 0x22, 0x33, 0x44, 0x55, 0xc0 } };
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_LE_SET_RANDOM_ADDRESS, random_addr_encode, &addr);

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected status 0x%02x", fut.result);
	TEST_ASSERT(fut.data != NULL, "No response");
	net_buf_unref(fut.data);
}

static void test_failure_status(void)
{
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	bt_hci_cmd_op_init(&op, UNKNOWN_OPCODE, NULL, NULL);

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result != 0, "Unknown command succeeded");
	TEST_ASSERT(fut.data == NULL, "Response delivered with status 0x%02x", fut.result);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation still pending");
}

static void test_fire_and_forget(void)
{
	struct bt_hci_cmd_op op;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_async(&op, NULL);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	for (int i = 0; bt_hci_cmd_op_is_pending(&op); i++) {
		TEST_ASSERT(i < 200, "Operation never completed");
		k_sleep(K_MSEC(10));
	}
}

static void test_callback(void)
{
	struct bt_hci_cmd_op_cb op;
	int err;

	bt_hci_cmd_op_cb_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_cb(&op, NULL, cmd_cb, &cb_sem);
	TEST_ASSERT(err == -EINVAL, "Unexpected result %d for a NULL workqueue", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op.op), "Operation pending after a failed send");

	err = bt_hci_cmd_send_cb(&op, &workq, cmd_cb, &cb_sem);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = k_sem_take(&cb_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Callback not invoked (err %d)", err);
	TEST_ASSERT(cb_status == 0, "Unexpected status 0x%02x", cb_status);
	TEST_ASSERT(cb_thread == k_work_queue_thread_get(&workq),
		    "Callback ran on the wrong thread");
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op.op), "Operation still pending");

	bt_hci_cmd_op_cb_init(&op, UNKNOWN_OPCODE, NULL, NULL);

	err = bt_hci_cmd_send_cb(&op, &workq, cmd_cb, &cb_sem);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = k_sem_take(&cb_sem, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Callback not invoked (err %d)", err);
	TEST_ASSERT(cb_status != 0, "Unknown command succeeded");
}

static void test_busy(void)
{
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	struct bt_future other;
	int err;

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	/* Keep the TX processor from dispatching between the two sends */
	k_sched_lock();

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d)", err);

	err = bt_hci_cmd_send_async(&op, &other);
	TEST_ASSERT(err == -EBUSY, "Unexpected result %d for a pending operation", err);

	k_sched_unlock();

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Wait failed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected status 0x%02x", fut.result);
	net_buf_unref(fut.data);
}

static void test_pool_exhausted(void)
{
	struct net_buf *held[32];
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	size_t count = 0;
	int err;

	/* Take every command buffer so that the operation has to wait */
	while (count < ARRAY_SIZE(held)) {
		held[count] = bt_hci_cmd_alloc(K_NO_WAIT);
		if (held[count] == NULL) {
			break;
		}
		count++;
	}
	TEST_ASSERT(count > 0 && count < ARRAY_SIZE(held), "Unexpected pool size %zu", count);

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == 0, "Send failed (err %d) with the pool exhausted", err);
	TEST_ASSERT(bt_hci_cmd_op_is_pending(&op), "Operation not pending");

	k_sleep(K_MSEC(100));
	TEST_ASSERT(!bt_future_is_done(&fut), "Command completed without a buffer");
	TEST_ASSERT(bt_hci_cmd_op_is_pending(&op), "Operation not pending");

	/* Freeing one buffer must get the operation dispatched */
	net_buf_unref(held[--count]);

	err = bt_future_wait(&fut, WAIT_TIMEOUT);
	TEST_ASSERT(err == 0, "Command not dispatched after a buffer was freed (err %d)", err);
	TEST_ASSERT(fut.result == 0, "Unexpected status 0x%02x", fut.result);
	net_buf_unref(fut.data);

	while (count > 0) {
		net_buf_unref(held[--count]);
	}
}

static void test_host_down(void)
{
	struct bt_hci_cmd_op_cb op_cb;
	struct bt_hci_cmd_op op;
	struct bt_future fut;
	int err;

	err = bt_disable();
	TEST_ASSERT(err == 0, "Disable failed (err %d)", err);

	bt_hci_cmd_op_init(&op, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_async(&op, &fut);
	TEST_ASSERT(err == -EHOSTDOWN, "Unexpected result %d with Bluetooth disabled", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op), "Operation pending after a failed send");

	bt_hci_cmd_op_cb_init(&op_cb, BT_HCI_OP_READ_BD_ADDR, NULL, NULL);

	err = bt_hci_cmd_send_cb(&op_cb, &workq, cmd_cb, &cb_sem);
	TEST_ASSERT(err == -EHOSTDOWN, "Unexpected result %d with Bluetooth disabled", err);
	TEST_ASSERT(!bt_hci_cmd_op_is_pending(&op_cb.op), "Operation pending after a failed send");
}

static void test_main(void)
{
	int err;

	k_work_queue_init(&workq);
	k_work_queue_start(&workq, workq_stack, K_THREAD_STACK_SIZEOF(workq_stack),
			   K_PRIO_PREEMPT(10), NULL);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Enable failed (err %d)", err);

	test_future();
	test_encoder();
	test_failure_status();
	test_fire_and_forget();
	test_callback();
	test_busy();
	test_pool_exhausted();
	test_host_down();

	TEST_PASS_AND_EXIT("HCI command async test passed");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "hci_cmd_async",
		.test_descr = "Asynchronous HCI command API",
		.test_main_f = test_main,
	},
	BSTEST_END_MARKER
};

static struct bst_test_list *test_hci_cmd_async_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_hci_cmd_async_install,
	NULL
};

int main(void)
{
	bst_main();

	return 0;
}
