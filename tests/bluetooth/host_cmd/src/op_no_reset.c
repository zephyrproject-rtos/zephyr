/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* bt_disable() with asynchronous HCI command operations under way. This needs
 * a controller that the Host does not reset, as the reset of bt_disable()
 * would not get past a command that is outstanding.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "future.h"
#include "hci_cmd_op.h"
#include "hci_core.h"

#include "driver.h"

/* Stack of a thread that sends commands, logging included */
#define SENDER_STACK_SIZE 2048U

/* The operation that most cases send, with its future */
static struct bt_hci_cmd_op_fut req;

static unsigned int encode_count;

static int encode(struct net_buf *buf, struct bt_hci_cmd_op *encoded)
{
	ARG_UNUSED(encoded);

	encode_count++;

	/* One parameter byte, of any value */
	(void)net_buf_add_u8(buf, 0x5aU);

	return 0;
}

static K_SEM_DEFINE(cb_sem, 0, 1);
static K_SEM_DEFINE(cb_gate, 0, 1);
static unsigned int cb_count;
static int cb_result;
static bool is_cb_gated;

static void op_cb(struct bt_hci_cmd_op *completed, int result)
{
	ARG_UNUSED(completed);

	cb_count++;
	cb_result = result;
	k_sem_give(&cb_sem);

	if (is_cb_gated) {
		is_cb_gated = false;
		/* Ended by the test, or by the reset of the gate in after() */
		(void)k_sem_take(&cb_gate, K_FOREVER);
	}
}

/* The test command with a parameter byte, and without one */
static const struct bt_hci_cmd_desc param_cmd = {
	.opcode = TEST_OPCODE,
	.encode = encode,
	.done = op_cb,
};

static const struct bt_hci_cmd_desc plain_cmd = {
	.opcode = TEST_OPCODE,
	.done = op_cb,
};

/* The work item of a test that sends from the system workqueue, and the end
 * of the wait for it
 */
static struct k_work syswq_work;
static K_SEM_DEFINE(work_done, 0, 1);

/* The threads of test_disable_races_cancel() */
#define RACE_THREADS 2U

static K_THREAD_STACK_ARRAY_DEFINE(race_stacks, RACE_THREADS, SENDER_STACK_SIZE);
static struct k_thread race_threads[RACE_THREADS];
static k_tid_t race_tids[RACE_THREADS];

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Lets go of a callback that a failed test left waiting */
	is_cb_gated = false;
	k_sem_reset(&cb_gate);

	test_cmd_bufs_release();

	/* The disable completes whatever the senders still wait for. A test
	 * that failed after its own disable left Bluetooth disabled.
	 */
	if (bt_is_ready()) {
		test_driver_release();
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}

	ARRAY_FOR_EACH(race_tids, i) {
		if (race_tids[i] != NULL) {
			k_thread_abort(race_tids[i]);
			race_tids[i] = NULL;
		}
	}

	test_workq_flush(&k_sys_work_q);
	test_bt_workq_flush();

	encode_count = 0U;
	cb_count = 0U;
	k_sem_reset(&cb_sem);
	k_sem_reset(&work_done);
}

ZTEST_SUITE(cmd_op_no_reset, test_without_reset, NULL, before, after, NULL);

/* bt_disable() hands every operation back once, whatever state it is in: with
 * the controller, encoded, queued, or waiting for its callback to run.
 */
static ZTEST(cmd_op_no_reset, test_disable_hands_back_all)
{
	static struct bt_hci_cmd_op_fut ops[3];
	static struct bt_hci_cmd_op cb_held_op;

	/* One whose callback cannot run yet, behind a blocked one */
	is_cb_gated = true;
	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The callback did not run");
	zassert_ok(bt_hci_cmd_send_cb(&cb_held_op, &plain_cmd), "Sending the operation failed");
	zassert_ok(test_rsp_wait(&test_cmd, 2U), "The second operation got no response");

	/* One with the controller, one encoded behind it, one without a buffer:
	 * the TX processor gives the operations their buffers before it sends
	 * a command
	 */
	test_cmd.delay = K_FOREVER;

	zassert_ok(bt_hci_cmd_send_async(&ops[0], &plain_cmd), "Sending the operation failed");
	zassert_ok(bt_hci_cmd_send_async(&ops[1], &plain_cmd), "Sending the operation failed");
	zassert_ok(test_cmd_wait(&test_cmd, 3U), "The third command was not sent");
	(void)test_cmd_bufs_hold();
	zassert_ok(bt_hci_cmd_send_async(&ops[2], &plain_cmd), "Sending the operation failed");
	test_settle();
	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	zassert_ok(bt_disable(), "Bluetooth disable failed");

	ARRAY_FOR_EACH(ops, i) {
		zassert_true(bt_future_is_done(&ops[i].fut),
			     "Operation %zu was not handed back", i);
		zassert_equal(ops[i].fut.result, -EHOSTDOWN, "Operation %zu gave %d", i,
			      ops[i].fut.result);
		zassert_false(bt_hci_cmd_op_is_pending(&ops[i].op), "Operation %zu is pending", i);
	}

	zassert_equal(test_cmd.count, 3U, "The controller got %u commands", test_cmd.count);

	/* The one that had completed keeps its result */
	zassert_equal(cb_count, 1U, "The callback ran %u times", cb_count);
	k_sem_give(&cb_gate);
	zassert_ok(k_sem_take(&cb_sem, TEST_TIMEOUT), "The pending callback did not run");
	zassert_equal(cb_count, 2U, "The callback ran %u times", cb_count);
	zassert_equal(cb_result, 0, "The completed one gave %d", cb_result);

	test_cmd_bufs_release();
	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	test_cmd_pool_check();
}

/* A synchronous sender on the system workqueue waits for a command credit where
 * the TX processor shares that queue. Nothing that a controller without a reset
 * could send wakes it up during bt_disable(), so the disable has to.
 */
static int disable_sender_err;

static void disable_sender_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	disable_sender_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	k_sem_give(&work_done);
}

static ZTEST(cmd_op_no_reset, test_disable_wakes_credit_waiter)
{
	Z_TEST_SKIP_IFDEF(CONFIG_BT_TX_PROCESSOR_THREAD);

	/* Taken back at once: the Host handles the event in this call */
	test_credit_revoke();

	/* Uses up what is left of the old credit, and then waits for a new one */
	k_work_init(&syswq_work, disable_sender_handler);
	zassert_true(k_work_submit(&syswq_work) >= 0, "Starting the sender failed");
	test_settle();
	zassert_equal(test_cmd.count, 0U, "The command was sent without a credit");

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(k_sem_take(&work_done, TEST_TIMEOUT), "The sender did not finish");
	zassert_equal(disable_sender_err, -EHOSTDOWN, "The sender got %d", disable_sender_err);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	test_cmd_pool_check();
}

/* bt_disable() purges an operation that has its command buffer and waits for
 * a command credit, and a thread of higher priority cancels it meanwhile: the
 * purge itself wakes that thread up when it completes the synchronous command
 * queued ahead of the operation. The operation is handed back once, by the
 * cancel or by the purge, never by both and never by neither.
 */
static int race_sync_err;
static int race_cancel_err;
static int race_disable_err;

static void race_cancel_entry(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	race_sync_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	race_cancel_err = bt_hci_cmd_op_cancel(&req.op);
}

static void race_disable_entry(void *p1, void *p2, void *p3)
{
	ARGS_UNUSED(p1, p2, p3);

	race_disable_err = bt_disable();
}

static ZTEST(cmd_op_no_reset, test_disable_races_cancel)
{
	unsigned int handback_cnt;

	/* The controller responds, and takes no further command */
	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	/* Both threads are preemptible, below the cooperative test thread, and
	 * the canceling one above the disabling one. The canceling one blocks
	 * on its synchronous command, which gets no credit.
	 */
	race_tids[0] = k_thread_create(&race_threads[0], race_stacks[0],
				       K_THREAD_STACK_SIZEOF(race_stacks[0]), race_cancel_entry,
				       NULL, NULL, NULL, K_PRIO_PREEMPT(1), 0, K_NO_WAIT);
	test_settle();

	zassert_ok(bt_hci_cmd_send_cb(&req.op, &param_cmd), "Sending the operation failed");
	test_settle();
	zassert_equal(encode_count, 1U, "The operation got no buffer");
	zassert_equal(test_cmd.count, 1U, "A command was sent without a credit");

	race_tids[1] = k_thread_create(&race_threads[1], race_stacks[1],
				       K_THREAD_STACK_SIZEOF(race_stacks[1]), race_disable_entry,
				       NULL, NULL, NULL, K_PRIO_PREEMPT(2), 0, K_NO_WAIT);
	zassert_ok(k_thread_join(race_tids[1], TEST_TIMEOUT), "bt_disable() did not return");
	zassert_ok(k_thread_join(race_tids[0], TEST_TIMEOUT), "The cancel did not return");
	zassert_ok(race_disable_err, "Bluetooth disable failed");
	zassert_equal(race_sync_err, -EHOSTDOWN, "The synchronous command gave %d (!= %d)",
		      race_sync_err, -EHOSTDOWN);

	/* A callback that is on its way has run */
	test_bt_workq_flush();

	handback_cnt = cb_count + ((race_cancel_err == 0) ? 1U : 0U);
	TC_PRINT("Cancel gave %d, %u callbacks\n", race_cancel_err, cb_count);
	zassert_equal(handback_cnt, 1U, "The operation was handed back %u times", handback_cnt);
	if (cb_count > 0U) {
		zassert_equal(cb_result, -EHOSTDOWN, "The result is %d", cb_result);
	}

	zassert_false(bt_hci_cmd_op_is_pending(&req.op), "The operation is pending");

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	test_cmd_pool_check();
}
