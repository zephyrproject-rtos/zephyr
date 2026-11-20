/*
 * Copyright (c) 2026 Tondo Ltd
 * Copyright (c) 2026 Process Mission
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
#include <smp_reassembly.h>
#include <string.h>

/*
 * The Bluetooth SMP transport is compiled into this test application by
 * including its source directly: its entry points are static, and the test
 * drives them without a Bluetooth controller. CMake removes the library copy
 * so that the same source is not linked twice.
 */
#include "../../../../../../subsys/mgmt/mcumgr/transport/src/smp_bt.c"

/*
 * Deterministic session-lifetime tests for the Bluetooth SMP transport.
 *
 * Connection objects use stable fake storage, requests are fed through
 * smp_bt_chr_write(), and disconnects through disconnected(). Client tests
 * replace host disconnect and reference handling. To make the ordering deterministic, a
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
static struct bt_conn *response_conn;

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
	response_conn = smp_bt_conn_from_pkt(nb);
	atomic_inc(&output_calls);
	smp_free_buf(nb, &smp_bt_transport);

	return 0;
}

/* struct bt_conn is opaque; host operations on these fake addresses are stubbed.
 */
static char conn_a_storage;
static char conn_b_storage;
static struct bt_conn *conn_a = (struct bt_conn *)&conn_a_storage;
static struct bt_conn *conn_b = (struct bt_conn *)&conn_b_storage;
static bool conn_a_active;
static bool conn_b_active;

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_CLIENT
static int disconnect_result;
static uint32_t disconnect_calls;
static uint32_t drop_calls;
static struct bt_conn *disconnect_conn;
static struct bt_conn *dropped_conn;

int __wrap_bt_conn_disconnect(struct bt_conn *conn, uint8_t reason)
{
	ARG_UNUSED(reason);
	disconnect_conn = conn;
	disconnect_calls++;
	return disconnect_result;
}

void __wrap_bt_conn_drop(struct bt_conn **conn)
{
	struct bt_conn *old = bt_conn_take(conn);

	if (old != NULL) {
		dropped_conn = old;
		drop_calls++;
	}
}
#endif

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
	response_conn = NULL;
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

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_CLIENT
	disconnect_result = 0;
	disconnect_calls = 0;
	drop_calls = 0;
	disconnect_conn = NULL;
	dropped_conn = NULL;
#endif
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

#ifdef CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY
static void write_ccc(struct bt_conn *conn, uint16_t value)
{
	const struct bt_gatt_attr *attr =
		&attr_smp_bt_svc[BT_SERVICE_ATTRIBUTE_SIZE + BT_CHARACTERISTIC_ATTRIBUTE_SIZE];
	struct bt_gatt_ccc_managed_user_data *ccc = attr->user_data;

	zassert_not_null(ccc->cfg_write);
	zassert_equal(ccc->cfg_write(conn, attr, value), sizeof(value));
}

static void start_partial_request(struct bt_conn *conn)
{
	uint8_t buf[32];
	uint16_t len;

	build_request(TEST_GROUP_A_ID, buf, sizeof(buf), &len);
	zassert_equal(smp_bt_chr_write(conn, NULL, buf, sizeof(struct smp_hdr), 0, 0),
		      sizeof(struct smp_hdr));
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), 1);
}

ZTEST(transport_bt, test_interleaved_connections_preserve_partial_request)
{
	uint8_t other[32];
	uint16_t other_len;
	const uint8_t body = 0xa0;
	struct smp_bt_user_data *ud;

	test_connect(conn_a);
	test_connect(conn_b);
	start_partial_request(conn_a);
	build_request(TEST_GROUP_B_ID, other, sizeof(other), &other_len);
	zassert_equal(smp_bt_chr_write(conn_b, NULL, other, other_len, 0, 0),
		      BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES));
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), 1);
	ud = smp_reassembly_get_ud(&smp_bt_transport);
	zassert_not_null(ud);
	zassert_equal_ptr(ud->conn, conn_a);
	zassert_equal(ud->id, conn_param_data_get(conn_a)->id);

	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0), sizeof(body));
	wait_for_calls(&output_calls, 1);
	zassert_equal(atomic_get(&group_a_calls), 1);
	zassert_equal(atomic_get(&group_b_calls), 0);
	zassert_equal_ptr(response_conn, conn_a);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 2);
	zassert_equal(atomic_get(&group_b_calls), 1);
	zassert_equal_ptr(response_conn, conn_b);
}

ZTEST(transport_bt, test_other_connection_cleanup_preserves_partial_request)
{
	const uint8_t body = 0xa0;

	test_connect(conn_a);
	test_connect(conn_b);
	start_partial_request(conn_a);
	write_ccc(conn_b, 0);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), 1);
	test_disconnect(conn_b);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), 1);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0), sizeof(body));
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_a);
}

ZTEST(transport_bt, test_ccc_reserved_bits_release_partial_request)
{
	test_connect(conn_a);
	test_connect(conn_b);
	start_partial_request(conn_a);
	write_ccc(conn_a, BIT(2));
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal(atomic_get(&group_b_calls), 1);
	zassert_equal_ptr(response_conn, conn_b);
}

ZTEST(transport_bt, test_owner_disconnect_releases_partial_request)
{
	test_connect(conn_a);
	test_connect(conn_b);
	start_partial_request(conn_a);
	test_disconnect(conn_a);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);

	test_connect(conn_a);
	enqueue_request(conn_a, TEST_GROUP_A_ID);
	wait_for_calls(&output_calls, 2);
	zassert_equal(atomic_get(&group_a_calls), 1);
	zassert_equal_ptr(response_conn, conn_a);
}

ZTEST(transport_bt, test_owner_error_releases_partial_request)
{
	const uint8_t oversized[] = {0xa0, 0xa0};

	test_connect(conn_a);
	test_connect(conn_b);
	start_partial_request(conn_a);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, oversized, sizeof(oversized), 0, 0),
		      BT_GATT_ERR(BT_ATT_ERR_VALUE_NOT_ALLOWED));
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);
}
#endif /* CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY */

#if defined(CONFIG_MCUMGR_TRANSPORT_BT_CLIENT) && defined(CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY)
static void connect_bridge_client(struct bt_conn *conn)
{
	test_connect(conn);
	struct conn_param_data *cpd = conn_param_data_get(conn);

	zassert_not_null(cpd);
	/* Model an established bridge without running Bluetooth service discovery. */
	cpd->state |= OUTGOING_CONNECTION | CONNECTED | DISCOVERED;
}

static void check_partial_packet(struct bt_conn *conn)
{
	struct smp_bt_user_data *ud = smp_reassembly_get_ud(&smp_bt_transport);
	struct conn_param_data *cpd = conn_param_data_get(conn);

	zassert_equal(smp_reassembly_expected(&smp_bt_transport), 1);
	zassert_not_null(ud);
	zassert_not_null(cpd);
	zassert_equal_ptr(ud->conn, conn);
	zassert_equal(ud->id, cpd->id);
}

static void build_response(uint8_t *buf, size_t size, uint16_t *len)
{
	build_request(TEST_GROUP_B_ID, buf, size, len);
	struct smp_hdr *hdr = (struct smp_hdr *)buf;

	hdr->nh_op = MGMT_OP_WRITE_RSP;
}

static void start_partial_response(struct bt_conn *conn)
{
	uint8_t buf[32];
	uint16_t len;

	build_response(buf, sizeof(buf), &len);
	zassert_equal(
		smp_bt_bridge_notify(conn, &bt_smp_subscribe_params, buf, sizeof(struct smp_hdr)),
		BT_GATT_ITER_CONTINUE);
	check_partial_packet(conn);
}

ZTEST(transport_bt, test_notification_preserves_other_connections_request)
{
	uint8_t response[32];
	uint16_t len;
	const uint8_t body = 0xa0;

	test_connect(conn_a);
	connect_bridge_client(conn_b);
	start_partial_request(conn_a);
	build_response(response, sizeof(response), &len);
	zassert_equal(smp_bt_bridge_notify(conn_b, &bt_smp_subscribe_params, response, len),
		      BT_GATT_ITER_CONTINUE);
	check_partial_packet(conn_a);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0), sizeof(body));
	wait_for_calls(&output_calls, 1);
	zassert_equal(atomic_get(&group_a_calls), 1);
	zassert_equal_ptr(response_conn, conn_a);

	start_partial_response(conn_b);
	test_disconnect(conn_b);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
}

ZTEST(transport_bt, test_gatt_write_preserves_other_connections_response)
{
	uint8_t request[32];
	uint16_t len;

	connect_bridge_client(conn_a);
	test_connect(conn_b);
	start_partial_response(conn_a);
	build_request(TEST_GROUP_B_ID, request, sizeof(request), &len);
	zassert_equal(smp_bt_chr_write(conn_b, NULL, request, len, 0, 0),
		      BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES));
	check_partial_packet(conn_a);
	write_ccc(conn_b, 0);
	check_partial_packet(conn_a);
	write_ccc(conn_a, 0);
	check_partial_packet(conn_a);
	test_disconnect(conn_a);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);
}

ZTEST(transport_bt, test_same_connection_request_and_response_do_not_mix)
{
	const uint8_t body = 0xa0;

	connect_bridge_client(conn_a);
	start_partial_request(conn_a);
	zassert_equal(smp_bt_bridge_notify(conn_a, &bt_smp_subscribe_params, &body, sizeof(body)),
		      BT_GATT_ITER_CONTINUE);
	check_partial_packet(conn_a);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0), sizeof(body));
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_a);

	start_partial_response(conn_a);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0),
		      BT_GATT_ERR(BT_ATT_ERR_INSUFFICIENT_RESOURCES));
	check_partial_packet(conn_a);
	test_disconnect(conn_a);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
}

ZTEST(transport_bt, test_subscription_end_releases_response_after_disconnect_error)
{
	struct bt_gatt_subscribe_params params = {.value_handle = 1U};

	connect_bridge_client(conn_a);
	test_connect(conn_b);
	start_partial_response(conn_a);
	disconnect_result = -EIO;
	zassert_equal(smp_bt_bridge_notify(conn_a, &params, NULL, 0), BT_GATT_ITER_STOP);
	zassert_equal(params.value_handle, 0U);
	zassert_equal(disconnect_calls, 1U);
	zassert_equal_ptr(disconnect_conn, conn_a);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);
}

ZTEST(transport_bt, test_subscription_end_preserves_same_connections_request)
{
	struct bt_gatt_subscribe_params params = {.value_handle = 1U};
	const uint8_t body = 0xa0;

	connect_bridge_client(conn_a);
	start_partial_request(conn_a);
	disconnect_result = -EIO;
	zassert_equal(smp_bt_bridge_notify(conn_a, &params, NULL, 0), BT_GATT_ITER_STOP);
	zassert_equal(disconnect_calls, 1U);
	check_partial_packet(conn_a);
	zassert_equal(smp_bt_chr_write(conn_a, NULL, &body, sizeof(body), 0, 0), sizeof(body));
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_a);
}

ZTEST(transport_bt, test_failed_bridge_teardown_releases_response)
{
	struct conn_param_data *cpd;

	connect_bridge_client(conn_a);
	test_connect(conn_b);
	cpd = conn_param_data_get(conn_a);
	start_partial_response(conn_a);
	disconnect_result = -EIO;
	smp_bt_bridge_disconnect(NULL, true);
	zassert_equal(disconnect_calls, 1U);
	zassert_equal_ptr(disconnect_conn, conn_a);
	zassert_equal(drop_calls, 1U);
	zassert_equal_ptr(dropped_conn, conn_a);
	zassert_is_null(cpd->conn);
	zassert_equal(cpd->id, 0U);
	zassert_equal(cpd->state, 0U);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	conn_a_active = false;
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);
}

ZTEST(transport_bt, test_notification_overflow_releases_owned_response)
{
	const uint8_t oversized[] = {0xa0, 0xa0};

	connect_bridge_client(conn_a);
	test_connect(conn_b);
	start_partial_response(conn_a);
	zassert_equal(smp_bt_bridge_notify(conn_a, &bt_smp_subscribe_params, oversized,
					   sizeof(oversized)),
		      BT_GATT_ITER_CONTINUE);
	zassert_equal(smp_reassembly_expected(&smp_bt_transport), -EINVAL);
	enqueue_request(conn_b, TEST_GROUP_B_ID);
	wait_for_calls(&output_calls, 1);
	zassert_equal_ptr(response_conn, conn_b);
}
#endif /* CONFIG_MCUMGR_TRANSPORT_BT_CLIENT && CONFIG_MCUMGR_TRANSPORT_BT_REASSEMBLY */
