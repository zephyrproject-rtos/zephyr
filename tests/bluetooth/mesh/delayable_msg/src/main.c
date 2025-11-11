/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/ztest.h>
#include <zephyr/net_buf.h>
#include <zephyr/bluetooth/mesh.h>
#include <zephyr/random/random.h>

#include "net.h"
#include "access.h"
#include "delayable_msg.h"

#define SRC_ADDR 0x0002
#define RX_ADDR  0xc000

static void start_cb(uint16_t duration, int err, void *cb_data);

struct bt_mesh_net bt_mesh;

static struct bt_mesh_msg_ctx gctx = {.net_idx = 0,
				      .app_idx = 0,
				      .addr = 0,
				      .recv_dst = RX_ADDR,
				      .uuid = NULL,
				      .recv_rssi = 0,
				      .recv_ttl = 0x05,
				      .send_rel = false,
				      .rnd_delay = true,
				      .send_ttl = 0x06};

static struct bt_mesh_send_cb send_cb = {
	.start = start_cb,
};

static bool is_fake_random;
static bool check_expectations;
static bool accum_mask;
static bool do_not_call_cb;
static uint16_t fake_random;
static uint8_t *buf_data;
static size_t buf_data_size;
static uint16_t id_mask;
static int cb_err_status;

K_SEM_DEFINE(delayed_msg_sent, 0, 1);

/**** Mocked functions ****/
int bt_mesh_access_send(struct bt_mesh_msg_ctx *ctx, struct net_buf_simple *buf, uint16_t src_addr,
			const struct bt_mesh_send_cb *cb, void *cb_data)
{
	if (check_expectations) {
		gctx.rnd_delay = false;
		ztest_check_expected_data(ctx, sizeof(struct bt_mesh_msg_ctx));
		gctx.rnd_delay = true;
		ztest_check_expected_value(src_addr);
		ztest_check_expected_data(cb, sizeof(struct bt_mesh_send_cb));
		ztest_check_expected_data(cb_data, sizeof(uint32_t));
		zexpect_mem_equal(buf->data, buf_data, buf_data_size, "Buffer data corrupted");
	}

	if (cb && !do_not_call_cb) {
		cb->start(0x0, cb_err_status, cb_data);
	}

	return cb_err_status;
}

int bt_rand(void *buf, size_t len)
{
	if (is_fake_random) {
		*(uint16_t *)buf = fake_random;
	} else {
		sys_rand_get(buf, len);
	}
	return 0;
}
/**** Mocked functions ****/

static void start_cb(uint16_t duration, int err, void *cb_data)
{

	zassert_equal(err, cb_err_status, "err: %d, cb_err_status: %d", err, cb_err_status);

	if (accum_mask) {
		id_mask |= 1 << *(uint16_t *)cb_data;
	} else {
		k_sem_give(&delayed_msg_sent);
	}
}

static void set_expectation(struct net_buf_simple *buf, uint32_t *buf_id)
{
	ztest_expect_data(bt_mesh_access_send, ctx, &gctx);
	ztest_expect_value(bt_mesh_access_send, src_addr, SRC_ADDR);
	ztest_expect_data(bt_mesh_access_send, cb, &send_cb);
	ztest_expect_data(bt_mesh_access_send, cb_data, buf_id);
	buf_data = buf->__buf;
	buf_data_size = buf->size;
	check_expectations = true;
}

static void tc_setup(void *fixture)
{
	is_fake_random = false;
	check_expectations = false;
	accum_mask = false;
	id_mask = 0;
	do_not_call_cb = false;
	cb_err_status = 0;
	k_sem_reset(&delayed_msg_sent);
	bt_mesh_delayable_msg_init();
}

static void tc_teardown(void *fixture)
{
	zassert_equal(gctx.net_idx, 0);
	zassert_equal(gctx.app_idx, 0);
	zassert_equal(gctx.addr, 0);
	zassert_equal(gctx.recv_dst, RX_ADDR);
	zassert_is_null(gctx.uuid);
	zassert_equal(gctx.recv_rssi, 0);
	zassert_equal(gctx.recv_ttl, 0x05);
	zassert_false(gctx.send_rel);
	zassert_true(gctx.rnd_delay);
	zassert_equal(gctx.send_ttl, 0x06);
}

ZTEST_SUITE(bt_mesh_delayable_msg, NULL, NULL, tc_setup, tc_teardown, NULL);

/* Simple single message sending with full size.
 */
ZTEST(bt_mesh_delayable_msg, test_single_sending)
{
	uint32_t buf_id = 0x55aa55aa;
	uint8_t *payload;

	NET_BUF_SIMPLE_DEFINE(buf, BT_MESH_TX_SDU_MAX);

	payload = net_buf_simple_add(&buf, BT_MESH_TX_SDU_MAX);
	for (int i = 0; i < BT_MESH_TX_SDU_MAX; i++) {
		payload[i] = i;
	}

	set_expectation(&buf, &buf_id);
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id));

	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");
}

/* The test checks that the delayed message mechanism sorts
 * the incoming messages according to the transmission start timestamp.
 */
ZTEST(bt_mesh_delayable_msg, test_self_sorting)
{
	uint8_t tx_data[20];
	uint32_t buf1_id = 1;
	uint32_t buf2_id = 2;
	uint32_t buf3_id = 3;
	uint32_t buf4_id = 4;

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, 20);
	NET_BUF_SIMPLE_DEFINE(buf3, 20);
	NET_BUF_SIMPLE_DEFINE(buf4, 20);

	memset(tx_data, 1, 20);
	memcpy(net_buf_simple_add(&buf1, 20), tx_data, 20);
	memset(tx_data, 2, 20);
	memcpy(net_buf_simple_add(&buf2, 20), tx_data, 20);
	memset(tx_data, 3, 20);
	memcpy(net_buf_simple_add(&buf3, 20), tx_data, 20);
	memset(tx_data, 4, 20);
	memcpy(net_buf_simple_add(&buf4, 20), tx_data, 20);

	is_fake_random = true;
	fake_random = 30;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf1_id));
	fake_random = 10;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf2_id));
	fake_random = 20;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf3, SRC_ADDR, &send_cb, &buf3_id));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf4, SRC_ADDR, &send_cb, &buf4_id));

	set_expectation(&buf2, &buf2_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf3, &buf3_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf1, &buf1_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
	set_expectation(&buf4, &buf4_id);
	zassert_ok(k_sem_take(&delayed_msg_sent, K_MSEC(100)),
		   "Delayed message has not been sent.");
}

/* The test checks that the delayed msg mechanism can allocate new context
 * if all contexts are in use by sending the message, that is the closest to
 * the tx time.
 */
ZTEST(bt_mesh_delayable_msg, test_ctx_reallocation)
{
	uint8_t tx_data[20];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;
	uint32_t buf_id5 = 4;

	NET_BUF_SIMPLE_DEFINE(buf, 20);

	memset(tx_data, 1, 20);
	memcpy(net_buf_simple_add(&buf, 20), tx_data, 20);

	accum_mask = true;
	is_fake_random = true;
	fake_random = 10;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id1));
	fake_random = 30;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id2));
	fake_random = 20;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id3));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id4));
	fake_random = 40;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id5));
	zassert_equal(id_mask, 0x0001, "Delayed message context reallocation was broken");
	k_sleep(K_MSEC(500));
	zassert_equal(id_mask, 0x001F);
}

/* The test checks that the delayed msg mechanism can allocate new chunks
 * if all chunks are in use by sending the other messages.
 */
ZTEST(bt_mesh_delayable_msg, test_chunk_reallocation)
{
	uint8_t tx_data[BT_MESH_TX_SDU_MAX];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, BT_MESH_TX_SDU_MAX);

	memset(tx_data, 1, BT_MESH_TX_SDU_MAX);
	memcpy(net_buf_simple_add(&buf1, 20), tx_data, 20);
	memcpy(net_buf_simple_add(&buf2, BT_MESH_TX_SDU_MAX), tx_data, BT_MESH_TX_SDU_MAX);

	accum_mask = true;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id1));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id2));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id3));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf_id4));
	zassert_equal(id_mask, 0x0007, "Delayed message chunks reallocation was broken");
	k_sleep(K_MSEC(500));
	zassert_equal(id_mask, 0x000F);
}

/* The test checks that the delayed msg mechanism can reschedule access messages
 * transport layes doesn't have enough memory or buffers at the moment.
 * Also it checks that the delayed msg mechanism can handle the other transport
 * layer errors without rescheduling appropriate access messages.
 */
ZTEST(bt_mesh_delayable_msg, test_cb_error_status)
{
	uint32_t buf_id = 0x55aa55aa;
	uint8_t tx_data[BT_MESH_TX_SDU_MAX];

	NET_BUF_SIMPLE_DEFINE(buf1, 20);
	NET_BUF_SIMPLE_DEFINE(buf2, 20);
	NET_BUF_SIMPLE_DEFINE(buf3, 20);

	memset(tx_data, 1, BT_MESH_TX_SDU_MAX);
	memcpy(net_buf_simple_add(&buf1, 20), tx_data, 20);
	memcpy(net_buf_simple_add(&buf2, 20), tx_data, 20);
	memcpy(net_buf_simple_add(&buf3, 20), tx_data, 20);

	cb_err_status = -ENOBUFS;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf1, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");

	cb_err_status = -EBUSY;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf2, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been sent.");

	cb_err_status = -EINVAL;
	do_not_call_cb = true;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf3, SRC_ADDR, &send_cb, &buf_id));
	zassert_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		   "Delayed message has not been handled.");
	cb_err_status = 0;
	zexpect_not_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		       "Delayed message has not been handled.");
}

/* The test checks that the delayed msg mechanism raises
 * the model message callback with the appropriate error code after
 * stopping the functionality.
 */
ZTEST(bt_mesh_delayable_msg, test_stop_handler)
{
	uint8_t tx_data[20];
	uint32_t buf_id1 = 0;
	uint32_t buf_id2 = 1;
	uint32_t buf_id3 = 2;
	uint32_t buf_id4 = 3;

	NET_BUF_SIMPLE_DEFINE(buf, 20);

	memset(tx_data, 1, 20);
	memcpy(net_buf_simple_add(&buf, 20), tx_data, 20);

	accum_mask = true;
	cb_err_status = -ENODEV;
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id1));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id2));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id3));
	zexpect_ok(bt_mesh_delayable_msg_manage(&gctx, &buf, SRC_ADDR, &send_cb, &buf_id4));
	bt_mesh_delayable_msg_stop();
	zexpect_not_ok(k_sem_take(&delayed_msg_sent, K_SECONDS(1)),
		       "Delayed message has been sent after stopping.");
	zassert_equal(id_mask, 0x000F, "Not all scheduled messages were handled after stopping");
}
