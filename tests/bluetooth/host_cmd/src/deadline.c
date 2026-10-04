/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* The HCI command deadline: what the Host does with a command that the
 * controller does not answer, with a controller that stops granting command
 * credits, and with a driver that fails to send a command.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "driver.h"

/* The command deadline of the Host */
#define DEADLINE_MS (10 * MSEC_PER_SEC)

/* A command sent from a work item, on whichever queue the test picks */
struct sender {
	struct k_work_delayable work;
	struct k_sem done;
	int err;
	int64_t done_time;
};

static void sender_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct sender *sender = CONTAINER_OF(dwork, struct sender, work);

	sender->err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	sender->done_time = k_uptime_get();
	k_sem_give(&sender->done);
}

static struct sender sender = {
	.work = Z_WORK_DELAYABLE_INITIALIZER(sender_handler),
	.done = Z_SEM_INITIALIZER(sender.done, 0, 1),
};

static K_SEM_DEFINE(connected_sem, 0, 1);
static struct bt_conn *test_conn;
static int connected_cmd_err;
static bool cmd_from_connected;

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(err);

	/* A connection of another suite */
	if (!cmd_from_connected) {
		return;
	}

	test_conn = bt_conn_ref(conn);

	/* This is the Bluetooth workqueue */
	connected_cmd_err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);

	k_sem_give(&connected_sem);
}

BT_CONN_CB_DEFINE(deadline_conn_callbacks) = {
	.connected = connected,
};

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	cmd_from_connected = false;
	k_sem_reset(&sender.done);

	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	test_reset_count = 0U;
	test_close_count = 0U;
}

/* Out of the state in which no command is accepted: bt_disable(), which must
 * not try to reset a controller that does not answer, and bt_enable().
 */
static void recover(void)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_equal(test_reset_count, 0U, "bt_disable() sent a reset");
	zassert_equal(test_close_count, 1U, "close() was called %u times", test_close_count);

	if (test_conn != NULL) {
		bt_conn_unref(test_conn);
		test_conn = NULL;
	}

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(test_reset_count, 1U, "bt_enable() sent %u resets", test_reset_count);
}

ZTEST_SUITE(cmd_deadline, test_with_reset, NULL, before, NULL, NULL);

/* A driver that fails to send a command has not sent it: the sender gets an
 * error at once and the next command goes through.
 */
static ZTEST(cmd_deadline, test_send_failure)
{
	int64_t start = k_uptime_get();
	int err;

	test_cmd.send_err = -ENOMEM;
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	zassert_equal(err, -EIO, "The command gave %d (!= %d)", err, -EIO);
	zassert_equal(test_cmd.count, 0U, "The controller got the command");
	zassert_true(k_uptime_get() - start < MSEC_PER_SEC, "The sender had to wait");

	test_cmd.send_err = 0;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* A command that gets no response ends with the deadline, and so does the one
 * queued behind it, whose sender is the system workqueue: the queue that
 * transmits commands when there is no TX processor thread. After that nothing
 * is accepted, a response that still arrives completes nothing, and a disable
 * and enable cycle brings Bluetooth back.
 */
static ZTEST(cmd_deadline, test_no_response)
{
	int64_t start = k_uptime_get();
	int64_t waited;
	int err;

	test_cmd.delay = K_FOREVER;
	(void)k_work_schedule(&sender.work, K_SECONDS(1));

	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	waited = k_uptime_get() - start;
	zassert_equal(err, -ETIMEDOUT, "The command gave %d (!= %d)", err, -ETIMEDOUT);
	zassert_within(waited, DEADLINE_MS, 100, "The command took %lld ms", waited);

	zassert_ok(k_sem_take(&sender.done, K_SECONDS(1)), "The queued command never returned");
	zassert_equal(sender.err, -EHOSTDOWN, "The queued command gave %d (!= %d)", sender.err,
		      -EHOSTDOWN);
	zassert_within(sender.done_time - start, DEADLINE_MS, 100,
		       "The queued command returned after %lld ms", sender.done_time - start);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* Nothing is accepted any more */
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	zassert_equal(err, -EHOSTDOWN, "A further command gave %d (!= %d)", err, -EHOSTDOWN);
	err = bt_hci_cmd_send(TEST_OPCODE, NULL);
	zassert_equal(err, -EHOSTDOWN, "A further command gave %d (!= %d)", err, -EHOSTDOWN);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The late response has nothing to complete */
	test_cmd_respond(&test_cmd);
	k_sleep(K_MSEC(100));

	recover();

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command failed after the recovery");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* A controller that answers with no command credit, and grants none later,
 * lets the queue behind the answered command stand still. The deadline covers
 * that too.
 */
static ZTEST(cmd_deadline, test_no_credit)
{
	int64_t start;
	int64_t waited;
	int err;

	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The answered command failed");

	start = k_uptime_get();
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	waited = k_uptime_get() - start;
	zassert_equal(err, -EHOSTDOWN, "The command gave %d (!= %d)", err, -EHOSTDOWN);
	zassert_within(waited, DEADLINE_MS, 100, "The command took %lld ms", waited);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	recover();
}

/* The Bluetooth workqueue runs synchronous senders, in callbacks and in the
 * Host's own event handlers. The deadline has to release those as well.
 */
static ZTEST(cmd_deadline, test_sender_on_bt_workq)
{
	int64_t start;
	int64_t waited;

	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");

	test_cmd.delay = K_FOREVER;
	cmd_from_connected = true;
	k_sem_reset(&connected_sem);

	start = k_uptime_get();
	test_connect();

	zassert_ok(k_sem_take(&connected_sem, K_MSEC(DEADLINE_MS + MSEC_PER_SEC)),
		   "The sender was not released");
	cmd_from_connected = false;
	waited = k_uptime_get() - start;
	zassert_equal(connected_cmd_err, -ETIMEDOUT, "The command gave %d (!= %d)",
		      connected_cmd_err, -ETIMEDOUT);
	zassert_within(waited, DEADLINE_MS, 100, "The command took %lld ms", waited);

	recover();
}

/* A response that the driver delivers from within send() completes the
 * command before send() returns. The deadline must not be left running for it.
 */
static ZTEST(cmd_deadline, test_response_within_send)
{
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	k_sleep(K_MSEC(DEADLINE_MS + MSEC_PER_SEC));

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command failed after an idle deadline period");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* The deadline of a command that is answered just in time must not end the
 * command that follows it.
 */
static ZTEST(cmd_deadline, test_response_just_in_time)
{
	test_cmd.delay = K_MSEC(DEADLINE_MS - 100);
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The slow command failed");

	test_cmd.delay = K_SECONDS(2);
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command after the slow one failed");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}
