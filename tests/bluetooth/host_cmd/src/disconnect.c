/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/* bt_conn_disconnect() does not wait for the controller. These cases cover
 * what becomes of the request: rejected, not sent, overtaken by the peer, and
 * responded to only after the connection is gone.
 */

#include <errno.h>
#include <stdint.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/hci_types.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "hci_core.h"

#include "driver.h"

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static struct bt_conn *conn;
static unsigned int connected_count;
static uint8_t disconnect_reason;
static bool is_running;

static void connected(struct bt_conn *new_conn, uint8_t err)
{
	ARG_UNUSED(err);

	/* A connection of another suite */
	if (!is_running) {
		return;
	}

	connected_count++;
	conn = bt_conn_ref(new_conn);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *old_conn, uint8_t reason)
{
	if (!is_running || old_conn != conn) {
		return;
	}

	disconnect_reason = reason;
	bt_conn_drop(&conn);
	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(disconnect_conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Holds up the Bluetooth workqueue, see
 * test_peer_first_no_credit_workq_busy
 */
static K_SEM_DEFINE(rx_gate, 0, 1);

static void rx_gate_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	/* Ended by the test, or by the reset of the gate in after() */
	(void)k_sem_take(&rx_gate, K_FOREVER);
}

static K_WORK_DEFINE(rx_gate_work, rx_gate_handler);

static void connect(void)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");
	test_connect();
	zassert_ok(k_sem_take(&connected_sem, TEST_TIMEOUT), "No connection");
}

static enum bt_conn_state conn_state(void)
{
	struct bt_conn_info info;

	zassert_ok(bt_conn_get_info(conn, &info), "No connection info");

	return info.state;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	test_driver_reset();
	is_running = true;
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");

	connect();
	connected_count = 0U;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	/* Lets go of the Bluetooth workqueue if a failed test left it held up */
	k_sem_reset(&rx_gate);
	test_cmd_bufs_release();

	/* Whatever is left of the connection goes with the disable. A test
	 * that failed between its own disable and enable left Bluetooth
	 * disabled.
	 */
	if (bt_is_ready()) {
		test_driver_release();
		zassert_ok(bt_disable(), "Bluetooth disable failed");
	}

	is_running = false;
	bt_conn_drop(&conn);

	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
}

ZTEST_SUITE(cmd_disconnect, test_with_reset, NULL, before, after, NULL);

/* A request that the controller rejects leaves the connection as it was: no
 * callback, and it can be disconnected again.
 */
static ZTEST(cmd_disconnect, test_rejected)
{
	test_disconnect.status = BT_HCI_ERR_CMD_DISALLOWED;

	/* Pending for the second request, as no time passes until the wait */
	test_disconnect.delay = K_MSEC(20);

	zassert_equal(bt_conn_disconnect(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN), -EINVAL,
		      "A reason that the controller would refuse was accepted");
	zassert_equal(test_disconnect.count, 0U, "The controller got the command");

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_equal(conn_state(), BT_CONN_STATE_DISCONNECTING, "Wrong state while requested");
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "A second request was refused");

	/* The rejection is dealt with on the Bluetooth workqueue */
	zassert_ok(test_rsp_wait(&test_disconnect, 1U), "The controller did not respond");
	test_bt_workq_flush();
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "Wrong state after the rejection");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U, "The connection was reported gone");
	zassert_equal(connected_count, 0U, "The connection was reported a second time");

	test_disconnect.status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused after a rejection");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_LOCALHOST_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);
}

/* A controller that does not know the connection any more has lost it without
 * saying so. The Host takes the connection down and says what it found.
 */
static ZTEST(cmd_disconnect, test_unknown_to_controller)
{
	test_disconnect.status = BT_HCI_ERR_UNKNOWN_CONN_ID;

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_UNKNOWN_CONN_ID, "Reason 0x%02x",
		      disconnect_reason);

	/* Done with the connection before the next one */
	test_bt_workq_flush();
	connect();
}

/* A command that the driver fails to send has not been sent: the connection
 * stays, and the next request goes through.
 */
static ZTEST(cmd_disconnect, test_send_failure)
{
	test_disconnect.send_err = -ENOMEM;

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	test_settle();
	zassert_equal(test_disconnect.count, 0U, "The controller got the command");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "Wrong state after the failure");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U, "The connection was reported gone");

	test_disconnect.send_err = 0;
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused after a failure");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);
}

/* A request that gets no command buffer before the deadline, as synchronous
 * senders keep the whole command pool for their responses, has not been sent:
 * the connection stays, and the next request goes through.
 */
static ZTEST(cmd_disconnect, test_no_buffer_until_deadline)
{
	ARRAY_FOR_EACH(test_cmd_bufs, i) {
		struct net_buf *buf = bt_hci_cmd_alloc(K_NO_WAIT);

		if (buf == NULL) {
			break;
		}

		zassert_ok(bt_hci_cmd_send_sync(TEST_OPCODE, buf, &test_cmd_bufs[i]),
			   "Command %zu failed", i);
	}

	zassert_is_null(test_cmd_bufs[ARRAY_SIZE(test_cmd_bufs) - 1U],
			"The Host has more command buffers than expected");

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_equal(conn_state(), BT_CONN_STATE_DISCONNECTING, "Wrong state while requested");

	/* The deadline gives the request up once its time has passed, which
	 * nothing signals
	 */
	(void)k_sleep(TEST_PAST_DEADLINE);
	zassert_equal(test_disconnect.count, 0U, "The controller got the command");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "Wrong state after the deadline");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U, "The connection was reported gone");
	zassert_equal(connected_count, 0U, "The connection was reported a second time");

	test_cmd_bufs_release();
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused after the deadline");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);
}

/* The peer disconnects while the request has no command buffer yet. The
 * connection object is used again at once, and the command of the old
 * connection must not come out of it.
 */
static ZTEST(cmd_disconnect, test_peer_first_no_buffer)
{
	(void)test_cmd_bufs_hold();

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	test_settle();

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);

	test_cmd_bufs_release();
	test_settle();
	connect();
	test_settle();

	zassert_equal(test_disconnect.count, 0U, "A command of the old connection was sent");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "The new connection is not up");
}

/* The same with the command encoded, waiting for the controller to accept
 * the next command.
 */
static ZTEST(cmd_disconnect, test_peer_first_no_credit)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The command was not sent");

	/* Encoded, behind the command that the controller holds */
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	test_settle();

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");

	/* The controller accepts commands again */
	test_cmd_respond(&test_cmd);
	test_settle();
	connect();
	test_settle();

	zassert_equal(test_disconnect.count, 0U, "A command of the old connection was sent");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "The new connection is not up");
}

/* The same with the Bluetooth workqueue still busy with something else when the
 * credit comes: the connection is gone for the controller, which may have given
 * the handle to a new connection already, and the Host has only seen the
 * priority part of the event so far.
 */
static ZTEST(cmd_disconnect, test_peer_first_no_credit_workq_busy)
{
	test_cmd.delay = K_FOREVER;
	zassert_ok(bt_hci_cmd_send(TEST_OPCODE, NULL), "Sending a command failed");
	zassert_ok(test_cmd_wait(&test_cmd, 1U), "The command was not sent");

	/* Encoded, behind the command that the controller holds */
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	test_settle();

	/* The workqueue is to be held up at the gate */
	zassert_true(bt_work_submit(&rx_gate_work) >= 0, "Blocking the workqueue failed");
	test_settle();

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	test_cmd_respond(&test_cmd);
	test_settle();

	zassert_equal(test_disconnect.count, 0U, "The command was sent after the disconnection");

	k_sem_give(&rx_gate);
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);
}

/* The connection goes away while the command is with the controller, and the
 * response comes afterwards. Until then the request keeps the connection
 * object; the completion drops the last reference to it, and the object is
 * free for the next connection.
 */
static void count_conn(struct bt_conn *any_conn, void *data)
{
	unsigned int *count = data;

	ARG_UNUSED(any_conn);

	(*count)++;
}

static ZTEST(cmd_disconnect, test_response_after_disconnection)
{
	const struct bt_conn *const first = conn;
	unsigned int objects = 0U;

	test_disconnect.status = BT_HCI_ERR_UNKNOWN_CONN_ID;

	/* The response comes after the disconnection that the test delivers */
	test_disconnect.delay = K_MSEC(500);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_ok(test_cmd_wait(&test_disconnect, 1U), "The command was not sent");

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);
	test_bt_workq_flush();

	/* The connection object is still taken */
	bt_conn_foreach(BT_CONN_TYPE_LE, count_conn, &objects);
	zassert_equal(objects, 1U, "The request did not keep the connection object");

	/* The completion of the request, on the Bluetooth workqueue, releases
	 * the object
	 */
	zassert_ok(test_rsp_wait(&test_disconnect, 1U), "The controller did not respond");
	test_bt_workq_flush();
	objects = 0U;
	bt_conn_foreach(BT_CONN_TYPE_LE, count_conn, &objects);
	zassert_equal(objects, 0U, "The connection object was not released");

	connect();
	zassert_equal_ptr(conn, first, "The new connection is in another object");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U,
		      "A late response disconnected the new connection");
}

/* The controller rejects the request, and the connection goes away by itself
 * before the Host has dealt with the rejection: the disconnection stands, and
 * the connection is not put back to connected.
 */
static ZTEST(cmd_disconnect, test_rejected_and_gone)
{
	unsigned int objects = 0U;

	test_disconnect.status = BT_HCI_ERR_CMD_DISALLOWED;
	test_disconnect.delay = K_FOREVER;

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_ok(test_cmd_wait(&test_disconnect, 1U), "The command was not sent");

	/* Both events arrive before the Bluetooth workqueue gets to either */
	k_sched_lock();
	test_cmd_respond(&test_disconnect);
	test_disconnect_complete(BT_HCI_ERR_CONN_TIMEOUT);
	k_sched_unlock();

	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_CONN_TIMEOUT, "Reason 0x%02x",
		      disconnect_reason);
	zassert_equal(connected_count, 0U, "The connection was reported a second time");

	/* The completion of the request, on the Bluetooth workqueue, releases
	 * the object
	 */
	test_bt_workq_flush();
	bt_conn_foreach(BT_CONN_TYPE_LE, count_conn, &objects);
	zassert_equal(objects, 0U, "The connection object was not released");
}

/* bt_disable() while the command of a request is with the controller: the
 * controller responds to the request before it gets the reset, and the
 * disable goes through.
 */
static ZTEST(cmd_disconnect, test_disable_after_request)
{
	/* The response comes once bt_disable() has been called */
	test_disconnect.delay = K_MSEC(100);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_ok(test_cmd_wait(&test_disconnect, 1U), "The command was not sent");

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	connect();
}

/* bt_disable() before the command of a request has been sent: the disable
 * takes the connection down, and the command is not needed any more.
 */
static ZTEST(cmd_disconnect, test_disable_overtakes_request)
{
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(k_sem_take(&disconnected_sem, TEST_TIMEOUT), "No disconnection");
	zassert_equal(test_disconnect.count, 0U, "The controller got %u commands",
		      test_disconnect.count);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	connect();
}
