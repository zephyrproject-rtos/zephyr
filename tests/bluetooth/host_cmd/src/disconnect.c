/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* bt_conn_disconnect() does not wait for the controller. These cases cover
 * what becomes of the request: rejected, not sent, overtaken by the peer, and
 * answered only after the connection is gone.
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

#include "driver.h"

static K_SEM_DEFINE(connected_sem, 0, 1);
static K_SEM_DEFINE(disconnected_sem, 0, 1);
static struct bt_conn *conn;
static unsigned int connected_count;
static uint8_t disconnect_reason;
static bool running;

static void connected(struct bt_conn *new_conn, uint8_t err)
{
	ARG_UNUSED(err);

	/* A connection of another suite */
	if (!running) {
		return;
	}

	connected_count++;
	conn = bt_conn_ref(new_conn);
	k_sem_give(&connected_sem);
}

static void disconnected(struct bt_conn *old_conn, uint8_t reason)
{
	if (!running || old_conn != conn) {
		return;
	}

	disconnect_reason = reason;
	bt_conn_unref(conn);
	conn = NULL;
	k_sem_give(&disconnected_sem);
}

BT_CONN_CB_DEFINE(disconnect_conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

/* Command buffers that a test keeps to itself */
static struct net_buf *held[32];

static void hold_buffers(void)
{
	ARRAY_FOR_EACH(held, i) {
		held[i] = bt_hci_cmd_alloc(K_NO_WAIT);
		if (held[i] == NULL) {
			return;
		}
	}

	zassert_unreachable("The Host has more command buffers than expected");
}

static void release_buffers(void)
{
	ARRAY_FOR_EACH(held, i) {
		if (held[i] != NULL) {
			net_buf_unref(held[i]);
			held[i] = NULL;
		}
	}
}

static void connect(void)
{
	zassert_ok(bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, NULL, 0U, NULL, 0U),
		   "Advertising failed to start");
	test_connect();
	zassert_ok(k_sem_take(&connected_sem, K_SECONDS(1)), "No connection");
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
	k_sem_reset(&connected_sem);
	k_sem_reset(&disconnected_sem);
	running = true;

	if (!bt_is_ready()) {
		zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	}

	connect();
	connected_count = 0U;
}

static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	release_buffers();
	test_driver_reset();

	/* Whatever is left of the connection goes with the disable */
	(void)bt_disable();
	running = false;

	if (conn != NULL) {
		bt_conn_unref(conn);
		conn = NULL;
	}
}

ZTEST_SUITE(cmd_disconnect, test_with_reset, NULL, before, after, NULL);

/* A request that the controller rejects leaves the connection as it was: no
 * callback, and it can be disconnected again.
 */
static ZTEST(cmd_disconnect, test_rejected)
{
	test_disconnect.status = BT_HCI_ERR_CMD_DISALLOWED;
	test_disconnect.delay = K_MSEC(20);

	zassert_equal(bt_conn_disconnect(conn, BT_HCI_ERR_LOCALHOST_TERM_CONN), -EINVAL,
		      "A reason that the controller would refuse was accepted");
	zassert_equal(test_disconnect.count, 0U, "The controller got the command");

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	zassert_equal(conn_state(), BT_CONN_STATE_DISCONNECTING, "Wrong state while requested");
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "A second request was refused");

	k_sleep(K_MSEC(50));
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "Wrong state after the rejection");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U, "The connection was reported gone");
	zassert_equal(connected_count, 0U, "The connection was reported a second time");

	test_disconnect.status = BT_HCI_ERR_SUCCESS;
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused after a rejection");
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
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
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_UNKNOWN_CONN_ID, "Reason 0x%02x",
		      disconnect_reason);

	k_sleep(K_MSEC(10));
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
	k_sleep(K_MSEC(10));
	zassert_equal(test_disconnect.count, 0U, "The controller got the command");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "Wrong state after the failure");
	zassert_equal(k_sem_count_get(&disconnected_sem), 0U, "The connection was reported gone");

	test_disconnect.send_err = 0;
	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused after a failure");
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);
}

/* The peer disconnects while the request has no command buffer yet. The
 * connection object is used again at once, and the command of the old
 * connection must not come out of it.
 */
static ZTEST(cmd_disconnect, test_peer_first_no_buffer)
{
	hold_buffers();

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	k_sleep(K_MSEC(10));

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);

	release_buffers();
	k_sleep(K_MSEC(10));
	connect();
	k_sleep(K_MSEC(10));

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
	k_sleep(K_MSEC(10));
	zassert_equal(test_cmd.count, 1U, "The controller got %u commands", test_cmd.count);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	k_sleep(K_MSEC(10));

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

	/* The controller accepts commands again */
	test_cmd_respond(&test_cmd);
	k_sleep(K_MSEC(10));
	connect();
	k_sleep(K_MSEC(10));

	zassert_equal(test_disconnect.count, 0U, "A command of the old connection was sent");
	zassert_equal(conn_state(), BT_CONN_STATE_CONNECTED, "The new connection is not up");
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
	test_disconnect.delay = K_MSEC(500);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	k_sleep(K_MSEC(10));
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);

	test_disconnect_complete(BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_REMOTE_USER_TERM_CONN, "Reason 0x%02x",
		      disconnect_reason);
	k_sleep(K_MSEC(10));

	/* The connection object is still taken */
	bt_conn_foreach(BT_CONN_TYPE_LE, count_conn, &objects);
	zassert_equal(objects, 1U, "The request did not keep the connection object");

	k_sleep(K_MSEC(500));
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
	k_sleep(K_MSEC(10));
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);

	/* Both events arrive before the Bluetooth workqueue gets to either */
	k_sched_lock();
	test_cmd_respond(&test_disconnect);
	test_disconnect_complete(BT_HCI_ERR_CONN_TIMEOUT);
	k_sched_unlock();

	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(disconnect_reason, BT_HCI_ERR_CONN_TIMEOUT, "Reason 0x%02x",
		      disconnect_reason);
	zassert_equal(connected_count, 0U, "The connection was reported a second time");

	k_sleep(K_MSEC(10));
	bt_conn_foreach(BT_CONN_TYPE_LE, count_conn, &objects);
	zassert_equal(objects, 0U, "The connection object was not released");
}

/* bt_disable() while the command of a request is with the controller: the
 * controller answers the request before it gets the reset, and the disable
 * goes through.
 */
static ZTEST(cmd_disconnect, test_disable_after_request)
{
	test_disconnect.delay = K_MSEC(100);

	zassert_ok(bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN),
		   "The request was refused");
	k_sleep(K_MSEC(10));
	zassert_equal(test_disconnect.count, 1U, "The controller got %u commands",
		      test_disconnect.count);

	zassert_ok(bt_disable(), "Bluetooth disable failed");
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");

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
	zassert_ok(k_sem_take(&disconnected_sem, K_SECONDS(1)), "No disconnection");
	zassert_equal(test_disconnect.count, 0U, "The controller got %u commands",
		      test_disconnect.count);

	test_driver_reset();
	zassert_ok(bt_enable(NULL), "Bluetooth init failed");
	connect();
}
