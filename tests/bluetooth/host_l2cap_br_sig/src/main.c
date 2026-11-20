/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/classic/l2cap_br.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/l2cap.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/slist.h>
#include <zephyr/sys/util.h>
#include <zephyr/ztest.h>

#include "host/conn_internal.h"
#include "host/hci_core.h"
#include "host/classic/l2cap_br_internal.h"

#define SIG_CID      0x0001U
#define CONNLESS_CID 0x0002U
#define ECHO_REQ     0x08U
#define ECHO_RSP     0x09U
#define CONNLESS_PSM 0x1001U
#define MAX_CALLS    2
#define MAX_DATA     8
#define SECURITY_PSM 0x1003U

/* What one callback invocation saw */
struct seen {
	uint8_t ident;
	uint16_t len;
	uint8_t data[MAX_DATA];
};

/* Calls seen by each of two registered callbacks */
struct cb_log {
	struct seen calls[MAX_CALLS];
	size_t count;
};

static struct cb_log echo_log[2];
static struct cb_log echo_rsp_log[2];
static struct cb_log connless_log[2];
static struct bt_conn *conn;
static struct bt_l2cap_br_chan security_chan[2];
static size_t security_disconnected;
static size_t security_released;
static size_t reused_encrypt_changes;
static bool reuse_released_channel;
static struct bt_l2cap_br_chan *allocated_security_chan;
static uint32_t release_work_busy;
static uint32_t disconnected_work_busy;
static bool disconnected_without_conn;
static size_t disconnected_at_release;
static struct k_work_q lifetime_workq;
static struct k_work_q host_workq;
K_THREAD_STACK_DEFINE(lifetime_stack, 2048);
K_THREAD_STACK_DEFINE(host_stack, CONFIG_BT_RX_STACK_SIZE);
K_SEM_DEFINE(lifetime_released, 0, 1);
K_SEM_DEFINE(delete_queue_blocked, 0, 1);
K_SEM_DEFINE(delete_queue_unblock, 0, 1);

/* Keep real workqueue execution without requiring bt_enable() and a controller. */
int __wrap_bt_work_submit(struct k_work *work)
{
	return k_work_submit_to_queue(&host_workq, work);
}

int __wrap_bt_work_schedule(struct k_work_delayable *work, k_timeout_t delay)
{
	return k_work_schedule_for_queue(&host_workq, work, delay);
}

int __wrap_bt_work_reschedule(struct k_work_delayable *work, k_timeout_t delay)
{
	return k_work_reschedule_for_queue(&host_workq, work, delay);
}

static void block_delete_queue(struct k_work *work)
{
	ARG_UNUSED(work);

	k_sem_give(&delete_queue_blocked);
	k_sem_take(&delete_queue_unblock, K_FOREVER);
}

static K_WORK_DEFINE(delete_queue_blocker, block_delete_queue);

static void security_chan_disconnected(struct bt_l2cap_chan *chan)
{
	ARG_UNUSED(chan);

	security_disconnected++;
}

static void reused_encrypt_change(struct bt_l2cap_chan *chan, uint8_t status)
{
	ARG_UNUSED(chan);
	ARG_UNUSED(status);

	reused_encrypt_changes++;
}

static const struct bt_l2cap_chan_ops reused_ops = {
	.encrypt_change = reused_encrypt_change,
};

static void security_chan_released(struct bt_l2cap_chan *chan)
{
	security_released++;
	if (reuse_released_channel) {
		struct bt_l2cap_br_chan *br_chan =
			CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan);

		/* Model the application immediately reusing and reattaching this storage. */
		chan->conn = conn;
		chan->ops = &reused_ops;
		/* Poison the flags to detect retry processing after release. */
		atomic_set(br_chan->flags, -1);
		sys_slist_append(&conn->channels, &chan->node);
	}
}

static const struct bt_l2cap_chan_ops security_ops = {
	.disconnected = security_chan_disconnected,
	.released = security_chan_released,
};

static void allocated_chan_disconnected(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan);

	disconnected_work_busy = k_work_busy_get(&br_chan->rtx_work.work) |
				 k_work_busy_get(&br_chan->retry_work.work);
	disconnected_without_conn = chan->conn == NULL;
	security_chan_disconnected(chan);
}

static void allocated_chan_released(struct bt_l2cap_chan *chan)
{
	struct bt_l2cap_br_chan *br_chan = CONTAINER_OF(chan, struct bt_l2cap_br_chan, chan);

	release_work_busy = k_work_busy_get(&br_chan->rtx_work.work) |
			    k_work_busy_get(&br_chan->retry_work.work);
	disconnected_at_release = security_disconnected;
	if (release_work_busy == 0) {
		k_free(br_chan);
	}
	k_sem_give(&lifetime_released);
}

static const struct bt_l2cap_chan_ops allocated_ops = {
	.disconnected = allocated_chan_disconnected,
	.released = allocated_chan_released,
};

static int security_accept(struct bt_conn *acl, struct bt_l2cap_server *server,
			   struct bt_l2cap_chan **chan)
{
	ARG_UNUSED(acl);
	ARG_UNUSED(server);

	*chan = allocated_security_chan != NULL ? &allocated_security_chan->chan :
		&security_chan[0].chan;
	return 0;
}

static struct bt_l2cap_server security_server = {
	.psm = SECURITY_PSM,
	.sec_level = BT_SECURITY_L3,
	.accept = security_accept,
};

NET_BUF_POOL_FIXED_DEFINE(pdu_pool, 1, 32, 0, NULL);
NET_BUF_POOL_FIXED_DEFINE(tx_pool, 1, BT_L2CAP_BR_ECHO_REQ_RESERVE + MAX_DATA,
			  CONFIG_BT_CONN_TX_USER_DATA_SIZE, NULL);

/* Records the data and then consumes it, as a callback parsing it would */
static void record(struct cb_log *rec, uint8_t ident, struct net_buf *buf)
{
	struct seen *s;

	zassert_true(rec->count < ARRAY_SIZE(rec->calls), "Too many calls");
	zassert_true(buf->len <= sizeof(s->data));

	s = &rec->calls[rec->count++];
	s->ident = ident;
	s->len = buf->len;
	memcpy(s->data, buf->data, buf->len);

	(void)net_buf_pull(buf, buf->len);
}

static void check(const struct cb_log *rec, size_t call, uint8_t ident, const char *data)
{
	const struct seen *s = &rec->calls[call];

	zassert_equal(s->ident, ident, "call %zu: ident %u", call, s->ident);
	zassert_equal(s->len, strlen(data), "call %zu: len %u", call, s->len);
	zassert_mem_equal(s->data, data, strlen(data), "call %zu: data", call);
}

static void echo_req_0(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[0], ident, buf);
}

static void echo_req_1(struct bt_conn *acl, uint8_t ident, struct net_buf *buf)
{
	record(&echo_log[1], ident, buf);
}

static void echo_rsp_0(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[0], 0, buf);
}

static void echo_rsp_1(struct bt_conn *acl, struct net_buf *buf)
{
	record(&echo_rsp_log[1], 0, buf);
}

static struct bt_l2cap_br_echo_cb echo_cb[] = {
	{ .req = echo_req_0, .rsp = echo_rsp_0 },
	{ .req = echo_req_1, .rsp = echo_rsp_1 },
};

static void connless_recv_0(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[0], 0, buf);
}

static void connless_recv_1(struct bt_conn *acl, uint16_t psm, struct net_buf *buf)
{
	zassert_equal(psm, CONNLESS_PSM);
	record(&connless_log[1], 0, buf);
}

/* One callback for all PSMs and one for CONNLESS_PSM, so both get the frame */
static struct bt_l2cap_br_connless_cb connless_cb[] = {
	{ .psm = 0, .sec_level = BT_SECURITY_L0, .recv = connless_recv_0 },
	{ .psm = CONNLESS_PSM, .sec_level = BT_SECURITY_L0, .recv = connless_recv_1 },
};

static struct net_buf *l2cap_pdu(uint16_t cid)
{
	struct net_buf *buf;

	buf = net_buf_alloc(&pdu_pool, K_NO_WAIT);
	zassert_not_null(buf);

	/* Length, filled in by send_pdu() */
	net_buf_add_le16(buf, 0U);
	net_buf_add_le16(buf, cid);

	return buf;
}

static void send_pdu(struct net_buf *buf)
{
	sys_put_le16(buf->len - 4U, buf->data);
	bt_l2cap_br_recv(conn, buf);
}

static void add_sig_cmd(struct net_buf *buf, uint8_t code, uint8_t ident, const char *data)
{
	net_buf_add_u8(buf, code);
	net_buf_add_u8(buf, ident);
	net_buf_add_le16(buf, strlen(data));
	net_buf_add_mem(buf, data, strlen(data));
}

static struct bt_l2cap_br_chan *sig_chan(void)
{
	struct bt_l2cap_chan *sig = bt_l2cap_br_lookup_rx_cid(conn, SIG_CID);

	zassert_not_null(sig);

	return CONTAINER_OF(sig, struct bt_l2cap_br_chan, chan);
}

static void clear_sig_pdus(void)
{
	sys_snode_t *node;

	while ((node = sys_slist_get(&sig_chan()->_pdu_tx_queue)) != NULL) {
		net_buf_unref(CONTAINER_OF(node, struct net_buf, node));
	}
}

/* Send an Echo Request and return its identifier, taken from the queued PDU */
static uint8_t send_echo_req(void)
{
	struct net_buf *buf;
	int err;

	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);
	net_buf_reserve(buf, BT_L2CAP_BR_ECHO_REQ_RESERVE);

	err = bt_l2cap_br_echo_req(conn, buf);
	zassert_equal(err, 0, "err %d", err);
	zassert_equal_ptr(sys_slist_peek_tail(&sig_chan()->_pdu_tx_queue), &buf->node);

	/* L2CAP header, then the signaling header code and identifier */
	return buf->data[5];
}

static void *setup(void)
{
	static const bt_addr_t addr = { .val = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 } };

	ARRAY_FOR_EACH_PTR(echo_cb, cb) {
		zassert_ok(bt_l2cap_br_echo_cb_register(cb));
	}

	ARRAY_FOR_EACH_PTR(connless_cb, cb) {
		zassert_ok(bt_l2cap_br_connless_register(cb));
	}

	zassert_ok(bt_l2cap_br_server_register(&security_server));

	k_work_queue_start(&host_workq, host_stack, K_THREAD_STACK_SIZEOF(host_stack),
			   K_PRIO_COOP(CONFIG_BT_RX_PRIO), NULL);
	k_thread_suspend(host_workq.thread_id);

	conn = bt_conn_add_br(&addr);
	zassert_not_null(conn);

	bt_l2cap_br_connected(conn);
	k_work_queue_start(&lifetime_workq, lifetime_stack, K_THREAD_STACK_SIZEOF(lifetime_stack),
			   K_PRIO_PREEMPT(1), NULL);

	return NULL;
}

static void before(void *fixture)
{
	ARG_UNUSED(fixture);

	clear_sig_pdus();
	memset(echo_log, 0, sizeof(echo_log));
	memset(echo_rsp_log, 0, sizeof(echo_rsp_log));
	memset(connless_log, 0, sizeof(connless_log));
	memset(security_chan, 0, sizeof(security_chan));
	ARRAY_FOR_EACH_PTR(security_chan, chan) {
		chan->chan.ops = &security_ops;
		chan->required_sec_level = BT_SECURITY_L2;
	}
	security_disconnected = 0;
	security_released = 0;
	reused_encrypt_changes = 0;
	reuse_released_channel = false;
	allocated_security_chan = NULL;
	release_work_busy = 0;
	disconnected_work_busy = 0;
	disconnected_without_conn = false;
	disconnected_at_release = 0;
	k_sem_reset(&lifetime_released);
	conn->state = BT_CONN_CONNECTED;
	conn->sec_level = BT_SECURITY_L1;
	conn->required_sec_level = BT_SECURITY_L1;
	conn->encrypt = 0;
	conn->br.features[1][0] |= BIT(0);
	atomic_clear_bit(conn->flags, BT_CONN_BR_PAIRING);
	atomic_clear_bit(conn->flags, BT_CONN_BR_GENERAL_BONDING);
}

/* Release the PDUs queued for sending, as there is no controller */
static void after(void *fixture)
{
	ARG_UNUSED(fixture);

	reuse_released_channel = false;
	ARRAY_FOR_EACH_PTR(security_chan, chan) {
		if (chan->chan.ops == &reused_ops) {
			bt_l2cap_chan_remove(conn, &chan->chan);
			chan->chan.conn = NULL;
			continue;
		}

		if (chan->chan.conn != NULL) {
			bt_l2cap_chan_remove(conn, &chan->chan);
			bt_l2cap_br_chan_del(&chan->chan);
		}
	}

	clear_sig_pdus();
}

ZTEST_SUITE(l2cap_br_sig, NULL, setup, before, after, NULL);

static void check_security_conn_req(struct bt_l2cap_br_chan *chan)
{
	struct net_buf *buf = SYS_SLIST_PEEK_HEAD_CONTAINER(&sig_chan()->_pdu_tx_queue, buf,
							 node);

	zassert_not_null(buf);
	zassert_equal(sys_slist_len(&sig_chan()->_pdu_tx_queue), 1);
	zassert_equal(buf->data[4], BT_L2CAP_CONN_REQ);
	zassert_equal(sys_get_le16(&buf->data[8]), SECURITY_PSM);
	zassert_equal(sys_get_le16(&buf->data[10]), chan->rx.cid);
}

ZTEST(l2cap_br_sig, test_connect_during_peer_pairing)
{
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);

	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));
	zassert_equal(security_chan[0].state, BT_L2CAP_CONNECTING);
	zassert_true(sys_slist_is_empty(&sig_chan()->_pdu_tx_queue));
	zassert_equal(conn->required_sec_level, BT_SECURITY_L1);
	zassert_false(atomic_test_bit(conn->flags, BT_CONN_BR_GENERAL_BONDING));

	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);
	check_security_conn_req(&security_chan[0]);

	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);
	check_security_conn_req(&security_chan[0]);
}

ZTEST(l2cap_br_sig, test_connect_during_peer_pairing_failure)
{
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));

	bt_conn_security_changed(conn, BT_HCI_ERR_AUTH_FAIL, BT_SECURITY_ERR_AUTH_FAIL);
	zassert_is_null(security_chan[0].chan.conn);
	zassert_equal(security_chan[0].state, BT_L2CAP_DISCONNECTED);
	zassert_equal(security_disconnected, 1);
	zassert_true(sys_slist_is_empty(&sig_chan()->_pdu_tx_queue));

	bt_conn_security_changed(conn, BT_HCI_ERR_AUTH_FAIL, BT_SECURITY_ERR_AUTH_FAIL);
	zassert_equal(security_disconnected, 1);
}

ZTEST(l2cap_br_sig, test_pending_security_level_insufficient)
{
	/* An existing local request also reaches the pending completion path. */
	conn->required_sec_level = BT_SECURITY_L3;
	security_chan[0].required_sec_level = BT_SECURITY_L3;
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));

	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);

	zassert_is_null(security_chan[0].chan.conn);
	zassert_equal(security_disconnected, 1);
	zassert_true(sys_slist_is_empty(&sig_chan()->_pdu_tx_queue));
}

ZTEST(l2cap_br_sig, test_pending_security_levels_per_channel)
{
	conn->required_sec_level = BT_SECURITY_L3;
	security_chan[0].required_sec_level = BT_SECURITY_L3;
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[1].chan, SECURITY_PSM));

	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);

	zassert_is_null(security_chan[0].chan.conn);
	zassert_equal(security_disconnected, 1);
	check_security_conn_req(&security_chan[1]);
}

ZTEST(l2cap_br_sig, test_connect_during_pairing_security_already_sufficient)
{
	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);

	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));
	check_security_conn_req(&security_chan[0]);
}

ZTEST(l2cap_br_sig, test_pending_security_released_channel_reused)
{
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	security_chan[0].required_sec_level = BT_SECURITY_L3;
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[0].chan, SECURITY_PSM));
	zassert_ok(bt_l2cap_br_chan_connect(conn, &security_chan[1].chan, SECURITY_PSM));
	reuse_released_channel = true;

	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);

	zassert_equal(security_disconnected, 1);
	zassert_equal(security_released, 1);
	zassert_equal(reused_encrypt_changes, 0);
	check_security_conn_req(&security_chan[1]);
}

static size_t exhaust_response_buffers(struct net_buf *rsp, struct net_buf **held, size_t capacity)
{
	struct net_buf_pool *pool = net_buf_pool_get(rsp->pool_id);
	struct net_buf *buf;
	size_t count = 0;

	while (count < capacity) {
		held[count] = net_buf_alloc(pool, K_NO_WAIT);
		if (held[count] == NULL) {
			break;
		}
		count++;
	}
	while (count < capacity) {
		buf = bt_conn_create_pdu_timeout(NULL, 0, K_NO_WAIT);
		if (buf == NULL) {
			break;
		}
		held[count] = buf;
		count++;
	}
	zassert_equal(count, capacity - 1);
	return count;
}

static void check_incoming_pending_security_insufficient(bool exhaust_buffers,
							 bool reuse_after_retry)
{
	struct net_buf *buf = l2cap_pdu(SIG_CID);
	struct net_buf *rsp;
	struct net_buf *held[CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT + CONFIG_BT_L2CAP_TX_BUF_COUNT];
	size_t held_count = 0;

	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	conn->required_sec_level = BT_SECURITY_L3;
	net_buf_add_u8(buf, BT_L2CAP_CONN_REQ);
	net_buf_add_u8(buf, 1);
	net_buf_add_le16(buf, sizeof(struct bt_l2cap_conn_req));
	net_buf_add_le16(buf, SECURITY_PSM);
	net_buf_add_le16(buf, 0x0040);
	send_pdu(buf);

	rsp = SYS_SLIST_PEEK_HEAD_CONTAINER(&sig_chan()->_pdu_tx_queue, rsp, node);
	zassert_not_null(rsp);
	zassert_equal(rsp->data[4], BT_L2CAP_CONN_RSP);
	zassert_equal(sys_get_le16(&rsp->data[12]), BT_L2CAP_BR_PENDING);

	if (exhaust_buffers) {
		held_count = exhaust_response_buffers(rsp, held, ARRAY_SIZE(held));
	}

	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);

	if (exhaust_buffers) {
		zassert_equal(security_released, 0);
		zassert_equal_ptr(security_chan[0].chan.conn, conn);
		zassert_equal(sys_slist_len(&sig_chan()->_pdu_tx_queue), 1);
		zassert_true(k_work_delayable_is_pending(&security_chan[0].retry_work));
		while (held_count != 0) {
			held_count--;
			net_buf_unref(held[held_count]);
		}

		/* The host queue is suspended while signaling is driven manually. */
		k_work_cancel_delayable(&security_chan[0].retry_work);
		reuse_released_channel = reuse_after_retry;
		security_chan[0].retry_work.work.handler(&security_chan[0].retry_work.work);
	}

	rsp = SYS_SLIST_PEEK_TAIL_CONTAINER(&sig_chan()->_pdu_tx_queue, rsp, node);
	zassert_not_null(rsp);
	zassert_equal(rsp->data[4], BT_L2CAP_CONN_RSP);
	zassert_equal(sys_get_le16(&rsp->data[12]), BT_L2CAP_BR_ERR_SEC_BLOCK);
	if (reuse_after_retry) {
		zassert_equal(atomic_get(security_chan[0].flags), -1);
		zassert_equal(reused_encrypt_changes, 0);
	} else {
		zassert_is_null(security_chan[0].chan.conn);
		zassert_is_null(bt_l2cap_br_lookup_tx_cid(conn, 0x0040));
	}
	zassert_equal(security_disconnected, 1);
	zassert_equal(security_released, 1);
	zassert_equal(conn->state, BT_CONN_CONNECTED);
}

ZTEST(l2cap_br_sig, test_incoming_pending_security_insufficient)
{
	check_incoming_pending_security_insufficient(false, false);
}

ZTEST(l2cap_br_sig, test_incoming_pending_security_insufficient_retry)
{
	check_incoming_pending_security_insufficient(true, false);
}

ZTEST(l2cap_br_sig, test_incoming_security_retry_released_channel_reused)
{
	check_incoming_pending_security_insufficient(true, true);
}

static struct bt_l2cap_br_chan *allocate_security_channel(void)
{
	struct bt_l2cap_br_chan *chan = k_calloc(1, sizeof(*chan));

	zassert_not_null(chan);
	chan->chan.ops = &allocated_ops;
	chan->required_sec_level = BT_SECURITY_L3;
	return chan;
}

static void check_workqueue_release(void)
{
	k_thread_resume(host_workq.thread_id);
	zassert_ok(k_sem_take(&lifetime_released, K_SECONDS(1)));
	zassert_true(k_work_queue_drain(&lifetime_workq, false) >= 0);
	zassert_true(k_work_queue_drain(&host_workq, false) >= 0);
	k_thread_suspend(host_workq.thread_id);
	if (release_work_busy != 0) {
		/* Keep a failing baseline safe: free only after its workqueue has drained. */
		k_free(allocated_security_chan);
	}
	allocated_security_chan = NULL;
	zassert_equal(release_work_busy, 0, "released() called with an embedded work still busy");
	zassert_equal(security_disconnected, 1);
	zassert_equal(disconnected_at_release, 1);
}

static void check_rtx_channel_release(k_timeout_t delay)
{
	allocated_security_chan = allocate_security_channel();
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	zassert_ok(bt_l2cap_br_chan_connect(conn, &allocated_security_chan->chan, SECURITY_PSM));
	zassert_true(k_work_reschedule_for_queue(&host_workq,
						  &allocated_security_chan->rtx_work, delay) >= 0);
	check_workqueue_release();
}

ZTEST(l2cap_br_sig, test_rtx_workqueue_channel_freed)
{
	check_rtx_channel_release(K_MSEC(1));
}

ZTEST(l2cap_br_sig, test_rtx_immediate_workqueue_channel_freed)
{
	check_rtx_channel_release(K_NO_WAIT);
}

ZTEST(l2cap_br_sig, test_security_failure_synchronous_release)
{
	allocated_security_chan = allocate_security_channel();
	/* Applications initialize public fields, not host-private work and flags. */
	memset(&allocated_security_chan->rtx_work, 0xa5,
	       sizeof(allocated_security_chan->rtx_work));
	memset(&allocated_security_chan->retry_work, 0xa5,
	       sizeof(allocated_security_chan->retry_work));
	memset(allocated_security_chan->flags, 0xa5, sizeof(allocated_security_chan->flags));
#if defined(CONFIG_BT_L2CAP_RET_FC)
	memset(&allocated_security_chan->ret_work, 0xa5,
	       sizeof(allocated_security_chan->ret_work));
	memset(&allocated_security_chan->monitor_work, 0xa5,
	       sizeof(allocated_security_chan->monitor_work));
#endif
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	zassert_ok(bt_l2cap_br_chan_connect(conn, &allocated_security_chan->chan, SECURITY_PSM));
	bt_conn_security_changed(conn, BT_HCI_ERR_AUTH_FAIL, BT_SECURITY_ERR_AUTH_FAIL);
	check_workqueue_release();
	zassert_false(disconnected_without_conn);
	zassert_equal(disconnected_work_busy, 0);
	zassert_true(sys_slist_is_empty(&sig_chan()->_pdu_tx_queue));
}

ZTEST(l2cap_br_sig, test_rtx_teardown_retains_acl_reference)
{
	struct bt_l2cap_br_chan *chan;
	struct net_buf *buf;
	struct k_work_sync sync = {0};
	atomic_val_t ref, ref_after;
	bool connection_retained, retry_idle, resources_clean;
	size_t disconnected;

	zassert_true(bt_work_submit(&delete_queue_blocker) >= 0);
	k_thread_resume(host_workq.thread_id);
	zassert_ok(k_sem_take(&delete_queue_blocked, K_SECONDS(1)));

	allocated_security_chan = allocate_security_channel();
	chan = allocated_security_chan;
	atomic_set_bit(conn->flags, BT_CONN_BR_PAIRING);
	zassert_ok(bt_l2cap_br_chan_connect(conn, &chan->chan, SECURITY_PSM));
	buf = net_buf_alloc(&tx_pool, K_NO_WAIT);
	zassert_not_null(buf);
	sys_slist_append(&chan->_pdu_tx_queue, &buf->node);
	sys_slist_append(&conn->l2cap_data_ready, &chan->_pdu_ready);
	atomic_set(&chan->_pdu_ready_lock, 1);
	ref = atomic_get(&conn->ref);
	zassert_true(k_work_reschedule_for_queue(&lifetime_workq, &chan->retry_work,
						K_SECONDS(10)) >= 0);
	zassert_true(k_work_reschedule_for_queue(&lifetime_workq, &chan->rtx_work,
						K_MSEC(1)) >= 0);
	k_work_flush_delayable(&chan->rtx_work, &sync);

	/* Inspect the detached channel while the host queue cannot run teardown. */
	connection_retained = chan->chan.conn == conn;
	retry_idle = k_work_delayable_busy_get(&chan->retry_work) == 0;
	resources_clean = sys_slist_is_empty(&chan->_pdu_tx_queue) &&
			  atomic_get(&chan->_pdu_ready_lock) == 0;
	disconnected = security_disconnected;
	ref_after = atomic_get(&conn->ref);

	k_sem_give(&delete_queue_unblock);
	check_workqueue_release();
	zassert_true(connection_retained, "Deferred channel lost its ACL connection");
	zassert_true(retry_idle, "Sibling retry work survived deletion");
	zassert_true(resources_clean, "Connection-owned TX resources survived deletion");
	zassert_equal(disconnected, 0, "disconnected() ran before final teardown");
	zassert_equal(ref_after, ref + 1, "Deferred teardown did not retain an ACL reference");
	zassert_equal(atomic_get(&conn->ref), ref, "Deferred teardown leaked an ACL reference");
	zassert_equal(disconnected_work_busy, 0);
	zassert_false(disconnected_without_conn);
}

ZTEST(l2cap_br_sig, test_security_retry_workqueue_channel_freed)
{
	struct net_buf *held[CONFIG_BT_L2CAP_SIG_TX_BUF_COUNT + CONFIG_BT_L2CAP_TX_BUF_COUNT];
	struct net_buf *buf = l2cap_pdu(SIG_CID);
	struct net_buf *rsp;
	size_t count;

	allocated_security_chan = allocate_security_channel();
	conn->encrypt = 1;
	conn->sec_level = BT_SECURITY_L2;
	conn->required_sec_level = BT_SECURITY_L3;
	net_buf_add_u8(buf, BT_L2CAP_CONN_REQ);
	net_buf_add_u8(buf, 1);
	net_buf_add_le16(buf, sizeof(struct bt_l2cap_conn_req));
	net_buf_add_le16(buf, SECURITY_PSM);
	net_buf_add_le16(buf, 0x0040);
	send_pdu(buf);
	rsp = SYS_SLIST_PEEK_HEAD_CONTAINER(&sig_chan()->_pdu_tx_queue, rsp, node);
	zassert_not_null(rsp);
	count = exhaust_response_buffers(rsp, held, ARRAY_SIZE(held));
	bt_conn_security_changed(conn, 0, BT_SECURITY_ERR_SUCCESS);
	zassert_true(k_work_delayable_is_pending(&allocated_security_chan->retry_work));
	k_work_cancel_delayable(&allocated_security_chan->retry_work);
	while (count != 0) {
		count--;
		net_buf_unref(held[count]);
	}
	zassert_true(k_work_reschedule_for_queue(&host_workq,
						  &allocated_security_chan->retry_work,
						  K_MSEC(1)) >= 0);
	check_workqueue_release();
	zassert_equal(conn->state, BT_CONN_CONNECTED);
	zassert_is_null(bt_l2cap_br_lookup_tx_cid(conn, 0x0040));
}

/* Each Echo Request callback sees exactly the data of that request, although
 * another request follows in the same signaling packet and the other callback
 * consumes the data.
 */
ZTEST(l2cap_br_sig, test_echo_req)
{
	struct net_buf *buf = l2cap_pdu(SIG_CID);

	add_sig_cmd(buf, ECHO_REQ, 1, "abc");
	add_sig_cmd(buf, ECHO_REQ, 2, "xy");
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_log, rec) {
		zassert_equal(rec->count, 2);
		check(rec, 0, 1, "abc");
		check(rec, 1, 2, "xy");
	}
}

/* Each Echo Response callback sees exactly the data of the response */
ZTEST(l2cap_br_sig, test_echo_rsp)
{
	struct net_buf *buf;
	uint8_t ident;

	ident = send_echo_req();

	buf = l2cap_pdu(SIG_CID);
	add_sig_cmd(buf, ECHO_RSP, ident, "pq");
	add_sig_cmd(buf, ECHO_REQ, (uint8_t)(ident + 1U), "z");
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(echo_rsp_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "pq");
	}
}

/* Each connectionless callback sees the whole data, although the other one consumes it */
ZTEST(l2cap_br_sig, test_connless_recv)
{
	struct net_buf *buf = l2cap_pdu(CONNLESS_CID);

	net_buf_add_le16(buf, CONNLESS_PSM);
	net_buf_add_mem(buf, "hello", strlen("hello"));
	send_pdu(buf);

	ARRAY_FOR_EACH_PTR(connless_log, rec) {
		zassert_equal(rec->count, 1);
		check(rec, 0, 0, "hello");
	}
}
