/*
 * Copyright (c) 2026 Tondo Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt.h>
#include <zephyr/mgmt/mcumgr/mgmt/mgmt_defines.h>
#include <zephyr/mgmt/mcumgr/transport/smp.h>
#include <smp_internal.h>
#include <string.h>

/*
 * The Bluetooth SMP transport is compiled into this test application by
 * including its source directly: its entry points are static, and the test
 * drives them without a Bluetooth controller. CONFIG_MCUMGR_TRANSPORT_BT is
 * left disabled so that the mcumgr_transport library does not build a second
 * copy of the same source.
 */
#include "../../../../../../subsys/mgmt/mcumgr/transport/src/smp_bt.c"

/*
 * Deterministic session-lifetime tests for the Bluetooth SMP transport.
 *
 * Connection objects are plain zeroed storage (the transport only stores and
 * compares their address), requests are fed through smp_bt_chr_write(), and
 * disconnects through disconnected(). To make the ordering deterministic, a
 * "blocker" request parks the SMP workqueue inside a handler, guaranteeing
 * that subsequently queued requests are still in the FIFO when the disconnect
 * runs.
 */

/* Test command groups; per-user IDs start at MGMT_GROUP_ID_PERUSER. */
#define TEST_CMD_ID 0
#define TEST_GROUP_A_ID MGMT_GROUP_ID_PERUSER
#define TEST_GROUP_B_ID (MGMT_GROUP_ID_PERUSER + 1)
#define TEST_BLOCKER_GROUP_ID (MGMT_GROUP_ID_PERUSER + 2)

/* Requests for group A must not run after their connection session ended. */
static atomic_t group_a_calls;
/* Requests for group B come from another connection and must survive a disconnect. */
static atomic_t group_b_calls;
/* Blocker requests park the workqueue until blocker_release_sem is given. */
static atomic_t blocker_calls;
static struct k_sem blocker_release_sem;
/* Number of responses routed to the (stubbed) output. */
static atomic_t output_calls;

static int group_a_write(struct smp_streamer *ctxt)
{
	atomic_inc(&group_a_calls);

	return 0;
}

static int group_b_write(struct smp_streamer *ctxt)
{
	atomic_inc(&group_b_calls);

	return 0;
}

static int blocker_write(struct smp_streamer *ctxt)
{
	atomic_inc(&blocker_calls);
	(void)k_sem_take(&blocker_release_sem, K_FOREVER);

	return 0;
}

static const struct mgmt_handler group_a_handlers[] = {
	[TEST_CMD_ID] = { .mh_write = group_a_write },
};

static const struct mgmt_handler group_b_handlers[] = {
	[TEST_CMD_ID] = { .mh_write = group_b_write },
};

static const struct mgmt_handler blocker_handlers[] = {
	[TEST_CMD_ID] = { .mh_write = blocker_write },
};

static struct mgmt_group group_a = {
	.mg_handlers = group_a_handlers,
	.mg_handlers_count = ARRAY_SIZE(group_a_handlers),
	.mg_group_id = TEST_GROUP_A_ID,
};

static struct mgmt_group group_b = {
	.mg_handlers = group_b_handlers,
	.mg_handlers_count = ARRAY_SIZE(group_b_handlers),
	.mg_group_id = TEST_GROUP_B_ID,
};

static struct mgmt_group blocker_group = {
	.mg_handlers = blocker_handlers,
	.mg_handlers_count = ARRAY_SIZE(blocker_handlers),
	.mg_group_id = TEST_BLOCKER_GROUP_ID,
};

/*
 * Response output stub: records the response and frees it. The real
 * smp_bt_tx_pkt() dereferences bt_conn objects, which are fake here, so it is
 * replaced after smp_bt_setup() has run.
 */
static int test_output(struct net_buf *nb)
{
	atomic_inc(&output_calls);
	smp_free_buf(nb, &smp_bt_transport);

	return 0;
}

/* Fake connection objects; the transport only stores and compares their address,
 * and struct bt_conn is opaque outside the Bluetooth host, so any stable address
 * works.
 */
static char conn_a_storage;
static char conn_b_storage;
static struct bt_conn *conn_a = (struct bt_conn *)&conn_a_storage;
static struct bt_conn *conn_b = (struct bt_conn *)&conn_b_storage;
static bool conn_a_active;
static bool conn_b_active;

static void test_connect(struct bt_conn *conn)
{
	if (conn == conn_a) {
		conn_a_active = true;
	} else {
		conn_b_active = true;
	}

	connected(conn, 0);
}

static void test_disconnect(struct bt_conn *conn)
{
	if (conn == conn_a) {
		conn_a_active = false;
	} else {
		conn_b_active = false;
	}

	disconnected(conn, 0);
}

/* Build a minimal SMP write request for the given group. */
static void build_request(uint16_t group_id, uint8_t *buf, size_t buf_size, uint16_t *req_len)
{
	struct smp_hdr *hdr = (struct smp_hdr *)buf;
	size_t body_len = 1; /* single CBOR item: empty map */

	zassert_true(buf_size >= sizeof(*hdr) + body_len, "request buffer too small");

	hdr->nh_op = MGMT_OP_WRITE;
	hdr->nh_version = 1;
	hdr->nh_flags = 0;
	hdr->nh_len = sys_cpu_to_be16(body_len);
	hdr->nh_group = sys_cpu_to_be16(group_id);
	hdr->nh_seq = 1;
	hdr->nh_id = TEST_CMD_ID;
	buf[sizeof(*hdr)] = 0xa0;

	*req_len = sizeof(*hdr) + body_len;
}

static void enqueue_request(struct bt_conn *conn, uint16_t group_id)
{
	uint8_t buf[32];
	uint16_t req_len;
	ssize_t rc;

	build_request(group_id, buf, sizeof(buf), &req_len);
	rc = smp_bt_chr_write(conn, NULL, buf, req_len, 0, 0);
	zassert_true(rc >= 0, "smp_bt_chr_write failed: %zd", rc);
}

/* Wait until the workqueue is parked inside the blocker handler. */
static void wait_for_blocker(void)
{
	int64_t deadline = k_uptime_get() + 1000;

	while (atomic_get(&blocker_calls) == 0) {
		zassert_true(k_uptime_get() < deadline, "blocker handler did not run");
		k_sleep(K_MSEC(5));
	}
}

/* Wait until the given counter reaches at least the expected value. */
static void wait_for_calls(atomic_t *counter, atomic_val_t expected)
{
	int64_t deadline = k_uptime_get() + 1000;

	while (atomic_get(counter) < expected) {
		zassert_true(k_uptime_get() < deadline, "handler did not run");
		k_sleep(K_MSEC(5));
	}
}

static void reset_state(void *fixture)
{
	atomic_set(&group_a_calls, 0);
	atomic_set(&group_b_calls, 0);
	atomic_set(&blocker_calls, 0);
	atomic_set(&output_calls, 0);
	k_sem_init(&blocker_release_sem, 0, 1);

	/* Drain any request left over from the previous test. */
	(void)smp_rx_clear(&smp_bt_transport);
	k_sleep(K_MSEC(20));

	if (conn_a_active) {
		test_disconnect(conn_a);
	}
	if (conn_b_active) {
		test_disconnect(conn_b);
	}
}

/* Register the test groups and stub the response output. Runs after SYS_INIT, so
 * smp_bt_setup() has already populated the transport functions.
 */
static void *suite_setup(void)
{
	mgmt_register_group(&group_a);
	mgmt_register_group(&group_b);
	mgmt_register_group(&blocker_group);
	smp_bt_transport.functions.output = test_output;

	return NULL;
}

ZTEST_SUITE(transport_bt, NULL, suite_setup, reset_state, NULL, NULL);

/*
 * A request that is still queued when its connection disconnects must not start
 * executing afterwards. The blocker parks the workqueue so the request is
 * guaranteed to still be in the FIFO when disconnected() runs.
 */
ZTEST(transport_bt, test_queued_request_not_executed_after_disconnect)
{
	test_connect(conn_a);

	enqueue_request(conn_a, TEST_BLOCKER_GROUP_ID);
	wait_for_blocker();

	enqueue_request(conn_a, TEST_GROUP_A_ID);
	test_disconnect(conn_a);

	/* Let the blocker finish; the workqueue drain continues with the queued request. */
	k_sem_give(&blocker_release_sem);
	k_sleep(K_MSEC(200));

	zassert_true(atomic_get(&group_a_calls) == 0,
		     "request queued before disconnect was executed after disconnect");
	zassert_true(atomic_get(&blocker_calls) == 1, "blocker request did not complete");
}

/*
 * Disconnecting one connection must not drop queued requests from another
 * connection, while still dropping the disconnecting connection's own requests.
 */
ZTEST(transport_bt, test_other_connection_requests_not_dropped)
{
	test_connect(conn_a);
	test_connect(conn_b);

	enqueue_request(conn_a, TEST_BLOCKER_GROUP_ID);
	wait_for_blocker();

	enqueue_request(conn_a, TEST_GROUP_A_ID);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	test_disconnect(conn_a);

	k_sem_give(&blocker_release_sem);
	wait_for_calls(&group_b_calls, 1);
	k_sleep(K_MSEC(200));

	zassert_true(atomic_get(&group_a_calls) == 0,
		     "stale request from disconnected connection was executed");
	zassert_true(atomic_get(&group_b_calls) == 1,
		     "valid request from another connection was dropped");
}

/* An ordinary request on a live connection must be processed normally. */
ZTEST(transport_bt, test_valid_request_executes)
{
	test_connect(conn_a);

	enqueue_request(conn_a, TEST_GROUP_A_ID);
	wait_for_calls(&group_a_calls, 1);

	zassert_true(atomic_get(&output_calls) >= 1, "response was not produced");
}

/*
 * A request whose handler is already running when the connection disconnects
 * completes; the transport cannot roll back an executing handler.
 */
ZTEST(transport_bt, test_already_running_request_completes)
{
	test_connect(conn_a);

	enqueue_request(conn_a, TEST_BLOCKER_GROUP_ID);
	wait_for_blocker();

	test_disconnect(conn_a);
	k_sem_give(&blocker_release_sem);

	wait_for_calls(&blocker_calls, 1);
	k_sleep(K_MSEC(100));

	zassert_true(atomic_get(&blocker_calls) == 1, "already-running handler did not complete");
}

/*
 * A queued request from a previous session must not run when the same connection
 * object is reused for a new session, and a fresh request on the new session
 * must still be processed.
 */
ZTEST(transport_bt, test_reused_connection_new_session)
{
	test_connect(conn_a);

	enqueue_request(conn_a, TEST_BLOCKER_GROUP_ID);
	wait_for_blocker();

	enqueue_request(conn_a, TEST_GROUP_A_ID);
	test_disconnect(conn_a);

	/* Reuse the same connection object for a new session. */
	test_connect(conn_a);

	k_sem_give(&blocker_release_sem);
	k_sleep(K_MSEC(200));

	zassert_true(atomic_get(&group_a_calls) == 0,
		     "stale request from previous session was executed");

	enqueue_request(conn_a, TEST_GROUP_A_ID);
	wait_for_calls(&group_a_calls, 1);
}

/*
 * Repeated disconnect cleanups must not exhaust the SMP packet pool: requests
 * dropped on disconnect are freed, and live requests still complete.
 */
ZTEST(transport_bt, test_no_buffer_leak_after_disconnect_cleanup)
{
	int i;

	for (i = 0; i < CONFIG_MCUMGR_TRANSPORT_NETBUF_COUNT * 2; i++) {
		test_connect(conn_a);
		enqueue_request(conn_a, TEST_BLOCKER_GROUP_ID);
		wait_for_blocker();
		enqueue_request(conn_a, TEST_GROUP_A_ID);
		test_disconnect(conn_a);
		k_sem_give(&blocker_release_sem);
		k_sleep(K_MSEC(20));
	}

	zassert_true(atomic_get(&group_a_calls) == 0, "stale requests were executed");
}
