/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"

#define LOG_MODULE_NAME mesh_test

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

/* Max number of messages that can be pending on RX at the same time */
#define RECV_QUEUE_SIZE 32

const struct bt_mesh_test_cfg *cfg;
struct bt_mesh_model *test_model;

static K_MEM_SLAB_DEFINE(msg_pool, sizeof(struct bt_mesh_test_msg),
			 RECV_QUEUE_SIZE, 4);
static K_QUEUE_DEFINE(recv);
struct bt_mesh_test_stats test_stats;
struct bt_mesh_msg_ctx test_send_ctx;

static int msg_rx(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf)
{
	size_t len = buf->len + BT_MESH_MODEL_OP_LEN(TEST_MSG_OP);
	static uint8_t prev_seq;
	struct bt_mesh_test_msg *msg;
	uint8_t seq = 0;

	if (buf->len) {
		seq = net_buf_simple_pull_u8(buf);
		if (prev_seq == seq) {
			FAIL("Received same message twice");
			return -EINVAL;
		}

		prev_seq = seq;
	}

	LOG_INF("Received packet 0x%02x:", seq);
	LOG_INF("\tlen: %d bytes", len);
	LOG_INF("\tsrc: 0x%04x", ctx->addr);
	LOG_INF("\tdst: 0x%04x", ctx->recv_dst);
	LOG_INF("\tttl: %u", ctx->recv_ttl);
	LOG_INF("\trssi: %d", ctx->recv_rssi);

	for (int i = 1; buf->len; i++) {
		if (net_buf_simple_pull_u8(buf) != (i & 0xff)) {
			FAIL("Invalid message content (byte %u)", i);
			return -EINVAL;
		}
	}

	test_stats.received++;

	if (k_mem_slab_alloc(&msg_pool, (void **)&msg, K_NO_WAIT)) {
		test_stats.recv_overflow++;
		return -EOVERFLOW;
	}

	msg->len = len;
	msg->seq = seq;
	msg->ctx = *ctx;

	k_queue_append(&recv, msg);

	return 0;
}

static const struct bt_mesh_model_op model_op[] = {
	{ TEST_MSG_OP, 0, msg_rx },
};
static struct bt_mesh_model_pub pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
};

static struct bt_mesh_cfg_cli cfg_cli;

static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL(TEST_MOD_ID, model_op, &pub, NULL),
};

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, BT_MESH_MODEL_NONE),
};

const struct bt_mesh_comp comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

const uint8_t test_net_key[16] = { 1, 2, 3 };
const uint8_t test_app_key[16] = { 4, 5, 6 };
const uint8_t test_va_uuid[16] = "Mesh Label UUID";

static void bt_enabled(void)
{
	static struct bt_mesh_prov prov;
	uint8_t status;
	int err;

	net_buf_simple_init(pub.msg, 0);

	err = bt_mesh_init(&prov, &comp);
	if (err) {
		FAIL("Initializing mesh failed (err %d)", err);
		return;
	}

	err = bt_mesh_provision(test_net_key, 0, 0, 0, cfg->addr, cfg->dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}

	LOG_INF("Mesh initialized");

	/* Self configure */

	err = bt_mesh_cfg_app_key_add(0, cfg->addr, 0, 0, test_app_key,
				      &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	err = bt_mesh_cfg_mod_app_bind(0, cfg->addr, cfg->addr, 0, TEST_MOD_ID,
				       &status);
	if (err || status) {
		FAIL("Mod app bind failed (err %d, status %u)", err, status);
		return;
	}

	err = bt_mesh_cfg_net_transmit_set(0, cfg->addr,
					   BT_MESH_TRANSMIT(2, 20), &status);
	if (err || status != BT_MESH_TRANSMIT(2, 20)) {
		FAIL("Net transmit set failed (err %d, status %u)", err,
		     status);
		return;
	}
}

void bt_mesh_test_setup(void)
{
	int err;

	test_model = &models[2];

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	bt_enabled();
}

void bt_mesh_test_timeout(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("Test timeout (not passed after %i seconds)",
		     HW_device_time / USEC_PER_SEC);
	}

	bs_trace_silent_exit(0);
}

void bt_mesh_test_cfg_set(const struct bt_mesh_test_cfg *my_cfg, int wait_time)
{
	bst_ticker_set_next_tick_absolute(wait_time * USEC_PER_SEC);
	bst_result = In_progress;
	cfg = my_cfg;
}

static struct bt_mesh_test_msg *blocking_recv(k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return 0;
	}

	return k_queue_get(&recv, timeout);
}

int bt_mesh_test_recv(uint16_t len, uint16_t dst, k_timeout_t timeout)
{
	struct bt_mesh_test_msg *msg = blocking_recv(timeout);

	if (!msg) {
		return -ETIMEDOUT;
	}

	if (len != msg->len) {
		FAIL("Recv: Invalid message length (%u, expected %u)", msg->len,
		     len);
		return -EINVAL;
	}

	if (dst != BT_MESH_ADDR_UNASSIGNED && dst != msg->ctx.recv_dst) {
		FAIL("Recv: Invalid dst 0x%04x, expected 0x%04x",
		     msg->ctx.recv_dst, dst);
		return -EINVAL;
	}

	k_mem_slab_free(&msg_pool, (void **)&msg);

	return 0;
}

int bt_mesh_test_recv_msg(struct bt_mesh_test_msg *msg, k_timeout_t timeout)
{
	struct bt_mesh_test_msg *queued = blocking_recv(timeout);

	if (!queued) {
		return -ETIMEDOUT;
	}

	*msg = *queued;

	k_mem_slab_free(&msg_pool, (void **)&queued);

	return 0;
}

int bt_mesh_test_recv_clear(void)
{
	struct bt_mesh_test_msg *queued;
	int count = 0;

	while ((queued = k_queue_get(&recv, K_NO_WAIT))) {
		k_mem_slab_free(&msg_pool, (void **)&queued);
		count++;
	}

	return count;
}

static void tx_started(uint16_t dur, int err, void *data)
{
	if (err) {
		FAIL("Couldn't start sending (err: %d)", err);
	}

	LOG_INF("Sending started");
}

static void tx_ended(int err, void *data)
{
	struct k_sem *sem = data;

	if (err) {
		FAIL("Send failed (%d)", err);
	}

	LOG_INF("Sending ended");

	k_sem_give(sem);
}

int bt_mesh_test_send_async(uint16_t addr, size_t len,
			    enum bt_mesh_test_send_flags flags,
			    const struct bt_mesh_send_cb *send_cb,
			    void *cb_data)
{
	const size_t mic_len =
		(flags & LONG_MIC) ? BT_MESH_MIC_LONG : BT_MESH_MIC_SHORT;
	static uint8_t count = 1;
	int err;

	test_send_ctx.addr = addr;
	test_send_ctx.send_rel = (flags & FORCE_SEGMENTATION);
	test_send_ctx.send_ttl = BT_MESH_TTL_DEFAULT;

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MSG_OP, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&buf, TEST_MSG_OP);

	if (len > BT_MESH_MODEL_OP_LEN(TEST_MSG_OP)) {
		net_buf_simple_add_u8(&buf, count);
	}

	/* Subtract the length of the opcode and the sequence ID */
	for (int i = 1; i < len - BT_MESH_MODEL_OP_LEN(TEST_MSG_OP); i++) {
		net_buf_simple_add_u8(&buf, i);
	}

	if (net_buf_simple_tailroom(&buf) < mic_len) {
		LOG_ERR("No room for MIC of len %u in %u byte buffer", mic_len,
			buf.len);
		return -EINVAL;
	}

	/* Seal the buffer to prevent accidentally long MICs: */
	buf.size = buf.len + mic_len;

	LOG_INF("Sending packet 0x%02x: %u %s to 0x%04x force seg: %u...",
		count, buf.len, (buf.len == 1 ? "byte" : "bytes"), addr,
		(flags & FORCE_SEGMENTATION));

	err = bt_mesh_model_send(test_model, &test_send_ctx, &buf, send_cb,
				 cb_data);
	if (err) {
		LOG_ERR("bt_mesh_model_send failed (err: %d)", err);
		return err;
	}

	count++;
	test_stats.sent++;
	return 0;
}

int bt_mesh_test_send(uint16_t addr, size_t len,
		      enum bt_mesh_test_send_flags flags, k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return bt_mesh_test_send_async(addr, len, flags, NULL, NULL);
	}

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	int64_t uptime = k_uptime_get();
	struct k_sem sem;
	int err;

	k_sem_init(&sem, 0, 1);
	err = bt_mesh_test_send_async(addr, len, flags, &send_cb, &sem);
	if (err) {
		return err;
	}

	err = k_sem_take(&sem, timeout);
	if (err) {
		LOG_ERR("Send timed out");
		return err;
	}

	LOG_INF("Sending completed (%lld ms)", k_uptime_delta(&uptime));

	return 0;
}
