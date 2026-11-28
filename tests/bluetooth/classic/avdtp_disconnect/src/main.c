/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <string.h>

#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/avdtp_internal.h"
#include "host/classic/l2cap_br_internal.h"

void bt_avdtp_l2cap_connected(struct bt_l2cap_chan *chan);
void bt_avdtp_l2cap_disconnected(struct bt_l2cap_chan *chan);

static struct bt_conn conn;
static struct bt_avdtp session;
static struct bt_avdtp_discover_params discover;
static struct bt_avdtp_discover_params another;
static struct net_buf *held_bufs[CONFIG_BT_MAX_CONN * 2];
static struct net_buf_pool *signalling_pool;
static bool exhaust_pool;
static bool pause_timeout;
static bool request_in_disconnect;
static int request_result;
static int reentrant_result;
static unsigned int sent;
static unsigned int completed;
static uint8_t completion_status;

static K_SEM_DEFINE(pool_exhausted, 0, 1);
static K_SEM_DEFINE(timeout_entered, 0, 1);
static K_SEM_DEFINE(allow_timeout, 0, 1);
static K_SEM_DEFINE(disconnect_entered, 0, 1);
static struct k_thread request_thread;
static K_THREAD_STACK_DEFINE(request_stack, 2048);

/* Use real work items and buffers, substituting only the transport and the
 * host queue so this test does not need an HCI controller.
 */
int bt_work_submit(struct k_work *work)
{
	return k_work_submit(work);
}

int bt_work_reschedule(struct k_work_delayable *work, k_timeout_t delay)
{
	if (pause_timeout) {
		pause_timeout = false;
		k_sem_give(&timeout_entered);
		k_sem_take(&allow_timeout, K_FOREVER);
	}

	return k_work_reschedule(work, delay);
}

int bt_l2cap_chan_connect(struct bt_conn *connection, struct bt_l2cap_chan *chan, uint16_t psm)
{
	zassert_equal(connection, &conn);
	zassert_equal(chan, &session.br_chan.chan);
	return 0;
}

int bt_l2cap_br_chan_send_cb(struct bt_l2cap_chan *chan, struct net_buf *buf, bt_conn_tx_cb_t cb,
			     void *user_data)
{
	zassert_equal(chan, &session.br_chan.chan);
	sent++;
	net_buf_unref(buf);
	return 0;
}

struct net_buf *real_bt_l2cap_create_pdu_timeout(struct net_buf_pool *pool, size_t reserve,
						 k_timeout_t timeout);

struct net_buf *bt_l2cap_create_pdu_timeout(struct net_buf_pool *pool, size_t reserve,
					    k_timeout_t timeout)
{
	signalling_pool = pool;
	if (exhaust_pool) {
		exhaust_pool = false;
		for (size_t i = 0; i < ARRAY_SIZE(held_bufs); i++) {
			held_bufs[i] = real_bt_l2cap_create_pdu_timeout(pool, reserve, K_NO_WAIT);
			zassert_not_null(held_bufs[i]);
		}

		k_sem_give(&pool_exhausted);
	}

	return real_bt_l2cap_create_pdu_timeout(pool, reserve, timeout);
}

static int request_complete(struct bt_avdtp_req *req, struct net_buf *buf)
{
	zassert_equal(req, &discover.req);
	zassert_is_null(buf);
	zassert_is_null(session.req);
	completed++;
	completion_status = req->status;
	return 0;
}

static void connected(struct bt_avdtp *avdtp)
{
	zassert_equal(avdtp, &session);
}

static void disconnected(struct bt_avdtp *avdtp)
{
	zassert_equal(avdtp, &session);
	if (request_in_disconnect) {
		/* L2CAP clears conn and state only after this callback returns. */
		zassert_not_null(avdtp->br_chan.chan.conn);
		zassert_equal(avdtp->br_chan.state, BT_L2CAP_CONNECTED);
		reentrant_result = bt_avdtp_discover(avdtp, &another);
	}
}

static const struct bt_avdtp_ops_cb ops = {
	.connected = connected,
	.disconnected = disconnected,
};

static void disconnect_handler(struct k_work *work)
{
	k_sem_give(&disconnect_entered);
	bt_avdtp_l2cap_disconnected(&session.br_chan.chan);
	session.br_chan.chan.conn = NULL;
	session.br_chan.state = BT_L2CAP_DISCONNECTED;
}

static K_WORK_DEFINE(disconnect_work, disconnect_handler);

static void drain_handler(struct k_work *work)
{
}

static K_WORK_DEFINE(drain_work, drain_handler);

static void drain_workqueue(void)
{
	struct k_work_sync sync;

	k_work_submit(&drain_work);
	k_work_flush(&drain_work, &sync);
}

static void disconnect_session(void)
{
	struct k_work_sync sync;

	k_work_submit(&disconnect_work);
	k_work_flush(&disconnect_work, &sync);
	drain_workqueue();
}

static void before(void *fixture)
{
	memset(&session, 0, sizeof(session));
	memset(&discover, 0, sizeof(discover));
	memset(&another, 0, sizeof(another));
	session.ops = &ops;
	discover.req.func = request_complete;
	exhaust_pool = false;
	pause_timeout = false;
	request_in_disconnect = false;
	sent = 0;
	completed = 0;
	reentrant_result = 0;
	k_sem_reset(&pool_exhausted);
	k_sem_reset(&timeout_entered);
	k_sem_reset(&allow_timeout);
	k_sem_reset(&disconnect_entered);

	zassert_ok(bt_avdtp_connect(&conn, &session));
	session.br_chan.state = BT_L2CAP_CONNECTED;
	session.br_chan.tx.mtu = BT_L2CAP_TX_MTU;
	bt_avdtp_l2cap_connected(&session.br_chan.chan);
}

static void after(void *fixture)
{
	struct k_work_sync sync;

	if (session.br_chan.chan.conn != NULL) {
		disconnect_session();
	}

	k_work_cancel_delayable_sync(&session.timeout_work, &sync);
}

static void discover_thread(void *p1, void *p2, void *p3)
{
	request_result = bt_avdtp_discover(&session, &discover);
}

ZTEST(avdtp_disconnect, test_disconnect_during_pdu_wait)
{
	exhaust_pool = true;
	k_thread_create(&request_thread, request_stack, K_THREAD_STACK_SIZEOF(request_stack),
			discover_thread, NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);

	/* The cooperative requester cannot be preempted after giving this
	 * semaphore; it runs until the real, exhausted PDU pool blocks it.
	 */
	zassert_ok(k_sem_take(&pool_exhausted, K_SECONDS(1)));
	zassert_is_null(session.req);
	disconnect_session();

	for (size_t i = 0; i < ARRAY_SIZE(held_bufs); i++) {
		net_buf_unref(held_bufs[i]);
		held_bufs[i] = NULL;
	}
	zassert_ok(k_thread_join(&request_thread, K_SECONDS(1)));
	drain_workqueue();

	zassert_equal(request_result, -ENOTCONN);
	zassert_is_null(session.req);
	zassert_equal(k_work_delayable_busy_get(&session.timeout_work), 0);
	zassert_equal(sent, 0);
	zassert_equal(completed, 0);

	for (size_t i = 0; i < ARRAY_SIZE(held_bufs); i++) {
		held_bufs[i] = net_buf_alloc(signalling_pool, K_NO_WAIT);
		zassert_not_null(held_bufs[i], "Rejected request leaked a PDU");
	}
	for (size_t i = 0; i < ARRAY_SIZE(held_bufs); i++) {
		net_buf_unref(held_bufs[i]);
		held_bufs[i] = NULL;
	}
}

ZTEST(avdtp_disconnect, test_disconnect_during_timeout_start)
{
	struct k_work_sync sync;
	unsigned int completed_before_timeout;

	pause_timeout = true;
	k_thread_create(&request_thread, request_stack, K_THREAD_STACK_SIZEOF(request_stack),
			discover_thread, NULL, NULL, NULL, K_PRIO_COOP(0), 0, K_NO_WAIT);
	zassert_ok(k_sem_take(&timeout_entered, K_SECONDS(1)));

	/* Attempt disconnect after publication but before the timer is armed.
	 * Cleanup must wait, otherwise the sender re-arms a cancelled timer.
	 */
	k_work_submit(&disconnect_work);
	zassert_ok(k_sem_take(&disconnect_entered, K_SECONDS(1)));
	completed_before_timeout = completed;
	k_sem_give(&allow_timeout);

	zassert_ok(k_thread_join(&request_thread, K_SECONDS(1)));
	k_work_flush(&disconnect_work, &sync);
	drain_workqueue();

	zassert_ok(request_result);
	zassert_equal(completed_before_timeout, 0);
	zassert_equal(completed, 1);
	zassert_equal(completion_status, BT_AVDTP_BAD_STATE);
	zassert_is_null(session.req);
	zassert_equal(k_work_delayable_busy_get(&session.timeout_work), 0);
	zassert_equal(sent, 1);
}

ZTEST(avdtp_disconnect, test_disconnect_clears_pending_request)
{
	zassert_ok(bt_avdtp_discover(&session, &discover));
	drain_workqueue();
	zassert_equal(sent, 1);
	zassert_equal(session.req, &discover.req);
	zassert_true(k_work_delayable_is_pending(&session.timeout_work));

	request_in_disconnect = true;
	disconnect_session();

	zassert_equal(reentrant_result, -ENOTCONN);
	zassert_is_null(session.req);
	zassert_equal(k_work_delayable_busy_get(&session.timeout_work), 0);
	zassert_equal(completed, 1);
	zassert_equal(completion_status, BT_AVDTP_BAD_STATE);
	zassert_equal(sent, 1);
}

ZTEST(avdtp_disconnect, test_busy_keeps_pending_request)
{
	zassert_ok(bt_avdtp_discover(&session, &discover));
	zassert_equal(bt_avdtp_discover(&session, &another), -EBUSY);
	drain_workqueue();

	zassert_equal(session.req, &discover.req);
	zassert_equal(sent, 1);
	disconnect_session();
	zassert_equal(completed, 1);
}

ZTEST(avdtp_disconnect, test_request_after_disconnect)
{
	disconnect_session();
	zassert_equal(bt_avdtp_discover(&session, &discover), -ENOTCONN);
	drain_workqueue();

	zassert_is_null(session.req);
	zassert_equal(k_work_delayable_busy_get(&session.timeout_work), 0);
	zassert_equal(sent, 0);
	zassert_equal(completed, 0);
}

ZTEST_SUITE(avdtp_disconnect, NULL, NULL, before, after, NULL);
