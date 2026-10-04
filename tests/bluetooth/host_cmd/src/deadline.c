/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* The HCI command deadline: what the Host does with a command that gets no
 * response, with a controller that stops granting command credits, and with
 * a driver that fails to send a command.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "hci_core.h"

#include "driver.h"

/* A command sent from a work item, on whichever queue the test picks */
struct work_sender {
	struct k_work_delayable work;
	struct k_sem done;
	int err;
	int64_t done_time;
};

static void sender_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct work_sender *sender = CONTAINER_OF(dwork, struct work_sender, work);

	sender->err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	sender->done_time = k_uptime_get();
	k_sem_give(&sender->done);
}

static struct work_sender sender;

static K_SEM_DEFINE(connected_sem, 0, 1);
static struct bt_conn *test_conn;
static int connected_cmd_err;
static bool is_cmd_from_connected;

static void connected(struct bt_conn *conn, uint8_t err)
{
	ARG_UNUSED(err);

	/* A connection of another suite */
	if (!is_cmd_from_connected) {
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

/* A command queued just as the deadline expires, see
 * test_not_extended_at_expiry
 */
static struct net_buf *expiry_buf;
static int expiry_err;

static void expiry_handler(struct k_timer *timer)
{
	ARG_UNUSED(timer);

	expiry_err = bt_hci_cmd_send(TEST_OPCODE, net_buf_take(&expiry_buf));
}

static K_TIMER_DEFINE(expiry_timer, expiry_handler, NULL);

/* A command of a work item that waits for a command buffer, see
 * test_tx_held_up_by_alloc
 */
static K_SEM_DEFINE(alloc_started, 0, 1);
static K_SEM_DEFINE(alloc_done, 0, 1);
static int alloc_err;

static void alloc_handler(struct k_work *work)
{
	struct net_buf *buf;

	ARG_UNUSED(work);

	k_sem_give(&alloc_started);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	alloc_err = bt_hci_cmd_send_sync(TEST_OPCODE, buf, NULL);

	k_sem_give(&alloc_done);
}

static K_WORK_DEFINE(alloc_work, alloc_handler);

static void *setup(void)
{
	k_work_init_delayable(&sender.work, sender_handler);
	(void)k_sem_init(&sender.done, 0U, 1U);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	test_reset_count = 0U;
	test_close_count = 0U;
}

static void after(void *fixture)
{
	struct k_work_sync sync;

	ARG_UNUSED(fixture);

	k_timer_stop(&expiry_timer);
	net_buf_drop(&expiry_buf);
	test_cmd_bufs_release();

	/* The disable completes whatever the work items still wait for. A
	 * test that failed in the middle of a recovery left Bluetooth disabled.
	 */
	if (bt_is_ready()) {
		test_driver_release();
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}
	(void)k_work_cancel_delayable_sync(&sender.work, &sync);
	(void)k_work_cancel_sync(&alloc_work, &sync);

	is_cmd_from_connected = false;
	bt_conn_drop(&test_conn);

	k_sem_reset(&sender.done);
	k_sem_reset(&connected_sem);
	k_sem_reset(&alloc_started);
	k_sem_reset(&alloc_done);
}

ZTEST_SUITE(cmd_deadline, test_with_reset, setup, before, after, NULL);

/* Out of the state in which no command is accepted: bt_disable(), which must
 * not try to reset a controller that does not respond, and bt_enable().
 */
static void recover(void)
{
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_equal(test_reset_count, 0U, "bt_disable() sent a reset");
	zassert_equal(test_close_count, 1U, "close() was called %u times", test_close_count);

	bt_conn_drop(&test_conn);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	zassert_equal(test_reset_count, 1U, "bt_enable() sent %u resets", test_reset_count);
}

/* A command queued just as the deadline expires is no step forward: the one
 * that gets no response still ends with the deadline.
 */
static ZTEST(cmd_deadline, test_not_extended_at_expiry)
{
	int64_t start = k_uptime_get();
	int64_t waited;
	int err;

	expiry_buf = bt_hci_cmd_alloc(K_NO_WAIT);
	zassert_not_null(expiry_buf, "No command buffer");
	expiry_err = -EINPROGRESS;

	test_cmd.delay = K_FOREVER;

	/* Started first, so it comes first in the tick of the deadline */
	k_timer_start(&expiry_timer, HCI_CMD_TIMEOUT, K_NO_WAIT);

	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	waited = k_uptime_get() - start;
	zassert_equal(err, -ETIMEDOUT, "The command gave %d (!= %d)", err, -ETIMEDOUT);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The command took %lld ms", waited);
	zassert_ok(expiry_err, "The command at the expiry was refused (err %d)", expiry_err);

	recover();
}

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

	/* The second command is queued while the first one waits for its
	 * response: a second into that wait, the first one has long reached
	 * the controller.
	 */
	zassert_equal(k_work_schedule(&sender.work, K_SECONDS(1)), 1,
		      "Scheduling the sender failed");

	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	waited = k_uptime_get() - start;
	zassert_equal(err, -ETIMEDOUT, "The command gave %d (!= %d)", err, -ETIMEDOUT);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The command took %lld ms", waited);

	zassert_ok(k_sem_take(&sender.done, TEST_TIMEOUT), "The queued command never returned");
	zassert_equal(sender.err, -EHOSTDOWN, "The queued command gave %d (!= %d)", sender.err,
		      -EHOSTDOWN);
	zassert_within(sender.done_time - start, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The queued command returned after %lld ms", sender.done_time - start);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* Nothing is accepted any more */
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	zassert_equal(err, -EHOSTDOWN, "A further command gave %d (!= %d)", err, -EHOSTDOWN);
	err = bt_hci_cmd_send(TEST_OPCODE, NULL);
	zassert_equal(err, -EHOSTDOWN, "A further command gave %d (!= %d)", err, -EHOSTDOWN);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The late response has nothing to complete, and its credit nothing to
	 * send
	 */
	test_cmd_respond(&test_cmd);
	test_settle();
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	recover();

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command failed after the recovery");
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);
}

/* A controller that responds with no command credit, and grants none later,
 * lets the queue behind that command stand still. The deadline covers that
 * too.
 */
static ZTEST(cmd_deadline, test_no_credit)
{
	int64_t start;
	int64_t waited;
	int err;

	test_cmd.ncmd = 0U;
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The first command failed");

	start = k_uptime_get();
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	waited = k_uptime_get() - start;
	zassert_equal(err, -EHOSTDOWN, "The command gave %d (!= %d)", err, -EHOSTDOWN);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The command took %lld ms", waited);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	recover();
}

/* A work item on the system workqueue waits for a command buffer while the
 * pool is held by a thread whose command is still to be sent. Without a TX
 * processor thread it is the system workqueue that sends the commands, so
 * that command is not sent and its buffer not freed. The deadline gives the
 * command up, not the controller.
 */
static ZTEST(cmd_deadline, test_tx_held_up_by_alloc)
{
	unsigned int count;
	int64_t start;
	int64_t waited;
	int err;

	count = test_cmd_bufs_hold();
	zassert_true(count > 0U, "The Host has no command buffer");

	zassert_true(k_work_submit(&alloc_work) >= 0, "Starting the work item failed");
	zassert_ok(k_sem_take(&alloc_started, TEST_TIMEOUT), "The work item did not start");

	start = k_uptime_get();
	err = bt_hci_cmd_send_sync(TEST_OPCODE, net_buf_take(&test_cmd_bufs[0]), NULL);
	waited = k_uptime_get() - start;

	test_cmd_bufs_release();

	zassert_ok(k_sem_take(&alloc_done, TEST_TIMEOUT), "The work item got no buffer");
	zassert_ok(alloc_err, "The command of the work item gave %d", alloc_err);

	if (IS_ENABLED(CONFIG_BT_TX_PROCESSOR_THREAD)) {
		zassert_ok(err, "The command gave %d", err);
		zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
		return;
	}

	zassert_equal(err, -ENOBUFS, "The command gave %d (!= %d)", err, -ENOBUFS);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The command took %lld ms", waited);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	/* The controller has not been given up on */
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
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
	is_cmd_from_connected = true;

	start = k_uptime_get();
	test_connect();

	zassert_ok(k_sem_take(&connected_sem, TEST_PAST_DEADLINE), "The sender was not released");
	waited = k_uptime_get() - start;
	zassert_equal(connected_cmd_err, -ETIMEDOUT, "The command gave %d (!= %d)",
		      connected_cmd_err, -ETIMEDOUT);
	zassert_within(waited, TEST_DEADLINE_MS, TEST_DEADLINE_SLACK_MS,
		       "The command took %lld ms", waited);

	recover();
}

/* A response that the driver delivers from within send() completes the
 * command before send() returns. The deadline must not be left running for it.
 */
static ZTEST(cmd_deadline, test_response_within_send)
{
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The command failed");

	/* A deadline that is left running expires only once its time has
	 * passed, and nothing signals the expiry.
	 */
	(void)k_sleep(TEST_PAST_DEADLINE);

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command failed after an idle deadline period");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* The deadline of a command whose response comes just in time must not end
 * the command that follows it.
 */
static ZTEST(cmd_deadline, test_response_just_in_time)
{
	test_cmd.delay = K_MSEC(TEST_DEADLINE_MS - TEST_DEADLINE_SLACK_MS);
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The slow command failed");

	/* Longer than what was left of the deadline of the first command */
	test_cmd.delay = K_MSEC(2U * TEST_DEADLINE_SLACK_MS);
	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command after the slow one failed");
	zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
}

/* A response that arrives at the moment of the deadline: the command either
 * completes or times out, never both and never neither, and the Host is left
 * in the state that goes with the outcome.
 */
static ZTEST(cmd_deadline, test_response_at_deadline)
{
	int err;

	test_cmd.delay = HCI_CMD_TIMEOUT;
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	TC_PRINT("The command %s\n", (err == 0) ? "completed" : "timed out");

	/* Whichever came second, in the same tick, has been dealt with */
	test_settle();

	if (err == 0) {
		test_cmd.delay = K_NO_WAIT;
		zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
			   "The command after the completed one failed");
		zassert_equal(test_cmd.count, 2U, "The controller got %u commands", test_cmd.count);
		return;
	}

	zassert_equal(err, -ETIMEDOUT, "The command gave %d (!= %d)", err, -ETIMEDOUT);
	err = bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL);
	zassert_equal(err, -EHOSTDOWN, "A further command gave %d (!= %d)", err, -EHOSTDOWN);
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	recover();

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL),
		   "The command failed after the recovery");
}

/* The latest response says whether the controller accepts another command. A
 * credit that it grants while a command is outstanding is taken back by the
 * response to that command if that response allows no further command, and
 * the next command waits until a response allows it.
 */
static ZTEST(cmd_deadline, test_credit_taken_back)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The controller got no command");

	/* A credit while the command is outstanding, and a command behind it */
	test_credit_grant();
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	test_settle();
	zassert_equal(test_cmd.count, 1U, "A command was sent while another was outstanding");

	test_cmd.ncmd = 0U;
	test_cmd_respond(&test_cmd);
	test_settle();
	zassert_equal(test_cmd.count, 1U, "A command was sent that the controller did not accept");

	test_cmd.delay = K_NO_WAIT;
	test_cmd.ncmd = 1U;
	test_credit_grant();
	zassert_ok(test_cmd_wait(&test_cmd, 2U), "The queued command was not sent");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
}

/* A response that allows no further command holds the next one back until a
 * later response allows it: here the Command Complete event for no command
 * that a controller sends for this alone.
 */
static ZTEST(cmd_deadline, test_credit_granted_later)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The controller got no command");

	test_cmd.ncmd = 0U;
	test_cmd_respond(&test_cmd);
	test_settle();
	zassert_equal(test_cmd.count, 1U, "A command was sent that the controller did not accept");

	test_cmd.delay = K_NO_WAIT;
	test_cmd.ncmd = 1U;
	test_credit_grant();
	zassert_ok(test_cmd_wait(&test_cmd, 2U), "The queued command was not sent");

	zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, NULL, NULL), "The next command failed");
}
