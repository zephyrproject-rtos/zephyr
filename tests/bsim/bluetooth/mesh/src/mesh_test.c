/*
 * Copyright (c) 2021 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "argparse.h"
#include <bs_pc_backchannel.h>
#include "mesh/crypto.h"

#define LOG_MODULE_NAME mesh_test

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME);

#include "common/bt_str.h"

/* Max number of messages that can be pending on RX at the same time */
#define RECV_QUEUE_SIZE 32

const struct bt_mesh_test_cfg *cfg;

K_MEM_SLAB_DEFINE_STATIC(msg_pool, sizeof(struct bt_mesh_test_msg),
			 RECV_QUEUE_SIZE, 4);
static K_QUEUE_DEFINE(recv);
struct bt_mesh_test_stats test_stats;
struct bt_mesh_msg_ctx test_send_ctx;
static void (*ra_cb)(uint8_t *, size_t);

static int msg_rx(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		   struct net_buf_simple *buf)
{
	size_t len = buf->len + BT_MESH_MODEL_OP_LEN(TEST_MSG_OP_1);
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

static int ra_rx(struct bt_mesh_model *mod, struct bt_mesh_msg_ctx *ctx,
		 struct net_buf_simple *buf)
{
	LOG_INF("\tlen: %d bytes", buf->len);
	LOG_INF("\tsrc: 0x%04x", ctx->addr);
	LOG_INF("\tdst: 0x%04x", ctx->recv_dst);
	LOG_INF("\tttl: %u", ctx->recv_ttl);
	LOG_INF("\trssi: %d", ctx->recv_rssi);

	if (ra_cb) {
		ra_cb(net_buf_simple_pull_mem(buf, buf->len), buf->len);
	}

	return 0;
}

static const struct bt_mesh_model_op model_op[] = {
	{ TEST_MSG_OP_1, 0, msg_rx },
	{ TEST_MSG_OP_2, 0, ra_rx },
	BT_MESH_MODEL_OP_END
};

int __weak test_model_pub_update(struct bt_mesh_model *mod)
{
	return -1;
}

int __weak test_model_settings_set(struct bt_mesh_model *model,
				   const char *name, size_t len_rd,
				   settings_read_cb read_cb, void *cb_arg)
{
	return -1;
}

void __weak test_model_reset(struct bt_mesh_model *model)
{
	/* No-op. */
}

static const struct bt_mesh_model_cb test_model_cb = {
	.settings_set = test_model_settings_set,
	.reset = test_model_reset,
};

static struct bt_mesh_model_pub pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
	.update = test_model_pub_update,
};

static const struct bt_mesh_model_op vnd_model_op[] = {
	BT_MESH_MODEL_OP_END,
};

int __weak test_vnd_model_pub_update(struct bt_mesh_model *mod)
{
	return -1;
}

int __weak test_vnd_model_settings_set(struct bt_mesh_model *model,
				       const char *name, size_t len_rd,
				       settings_read_cb read_cb, void *cb_arg)
{
	return -1;
}

void __weak test_vnd_model_reset(struct bt_mesh_model *model)
{
	/* No-op. */
}

static const struct bt_mesh_model_cb test_vnd_model_cb = {
	.settings_set = test_vnd_model_settings_set,
	.reset = test_vnd_model_reset,
};

static struct bt_mesh_model_pub vnd_pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
	.update = test_vnd_model_pub_update,
};

static struct bt_mesh_cfg_cli cfg_cli;

static struct bt_mesh_health_srv health_srv;
static struct bt_mesh_model_pub health_pub = {
	.msg = NET_BUF_SIMPLE(BT_MESH_TX_SDU_MAX),
};

#if defined(CONFIG_BT_MESH_SAR_CFG)
static struct bt_mesh_sar_cfg_cli sar_cfg_cli;
#endif

#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
static struct bt_mesh_priv_beacon_cli priv_beacon_cli;
#endif

#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
static struct bt_mesh_od_priv_proxy_cli priv_proxy_cli;
#endif

static struct bt_mesh_model models[] = {
	BT_MESH_MODEL_CFG_SRV,
	BT_MESH_MODEL_CFG_CLI(&cfg_cli),
	BT_MESH_MODEL_CB(TEST_MOD_ID, model_op, &pub, NULL, &test_model_cb),
	BT_MESH_MODEL_HEALTH_SRV(&health_srv, &health_pub),
#if defined(CONFIG_BT_MESH_SAR_CFG)
	BT_MESH_MODEL_SAR_CFG_SRV,
	BT_MESH_MODEL_SAR_CFG_CLI(&sar_cfg_cli),
#endif
#if defined(CONFIG_BT_MESH_PRIV_BEACONS)
	BT_MESH_MODEL_PRIV_BEACON_SRV,
	BT_MESH_MODEL_PRIV_BEACON_CLI(&priv_beacon_cli),
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_SRV)
	BT_MESH_MODEL_OD_PRIV_PROXY_SRV,
#endif
#if defined(CONFIG_BT_MESH_OD_PRIV_PROXY_CLI)
	BT_MESH_MODEL_OD_PRIV_PROXY_CLI(&priv_proxy_cli),
#endif
};

struct bt_mesh_model *test_model = &models[2];

static struct bt_mesh_model vnd_models[] = {
	BT_MESH_MODEL_VND_CB(TEST_VND_COMPANY_ID, TEST_VND_MOD_ID, vnd_model_op, &vnd_pub,
			     NULL, &test_vnd_model_cb),
};

struct bt_mesh_model *test_vnd_model = &vnd_models[0];

static struct bt_mesh_elem elems[] = {
	BT_MESH_ELEM(0, models, vnd_models),
};

const struct bt_mesh_comp comp = {
	.elem = elems,
	.elem_count = ARRAY_SIZE(elems),
};

const uint8_t test_net_key[16] = { 1, 2, 3 };
const uint8_t test_app_key[16] = { 4, 5, 6 };
const uint8_t test_va_uuid[16] = "Mesh Label UUID";

static void bt_mesh_device_provision_and_configure(void)
{
	uint8_t status;
	int err;

	err = bt_mesh_provision(test_net_key, 0, 0, 0, cfg->addr, cfg->dev_key);
	if (err == -EALREADY) {
		LOG_INF("Using stored settings");
		return;
	} else if (err) {
		FAIL("Provisioning failed (err %d)", err);
		return;
	}

	/* Self configure */

	err = bt_mesh_cfg_cli_app_key_add(0, cfg->addr, 0, 0, test_app_key, &status);
	if (err || status) {
		FAIL("AppKey add failed (err %d, status %u)", err, status);
		return;
	}

	err = bt_mesh_cfg_cli_mod_app_bind(0, cfg->addr, cfg->addr, 0, TEST_MOD_ID, &status);
	if (err || status) {
		FAIL("Mod app bind failed (err %d, status %u)", err, status);
		return;
	}

	err = bt_mesh_cfg_cli_net_transmit_set(0, cfg->addr, BT_MESH_TRANSMIT(2, 20), &status);
	if (err || status != BT_MESH_TRANSMIT(2, 20)) {
		FAIL("Net transmit set failed (err %d, status %u)", err,
		     status);
		return;
	}
}

void bt_mesh_device_setup(const struct bt_mesh_prov *prov, const struct bt_mesh_comp *comp)
{
	int err;

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	err = bt_mesh_init(prov, comp);
	if (err) {
		FAIL("Initializing mesh failed (err %d)", err);
		return;
	}

	if (IS_ENABLED(CONFIG_BT_SETTINGS)) {
		LOG_INF("Loading stored settings");
		if (IS_ENABLED(CONFIG_BT_MESH_USES_MBEDTLS_PSA)) {
			settings_load_subtree("itsemul");
		}
		settings_load_subtree("bt");
	}

	LOG_INF("Mesh initialized");
}

void bt_mesh_test_setup(void)
{
	static struct bt_mesh_prov prov;

	net_buf_simple_init(pub.msg, 0);
	net_buf_simple_init(vnd_pub.msg, 0);

	bt_mesh_device_setup(&prov, &comp);
	bt_mesh_device_provision_and_configure();
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

	/* Ensure those test devices will not drift more than
	 * 100ms for each other in emulated time
	 */
	tm_set_phy_max_resync_offset(100000);
}

static struct bt_mesh_test_msg *blocking_recv(k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return 0;
	}

	return k_queue_get(&recv, timeout);
}

int bt_mesh_test_recv(uint16_t len, uint16_t dst, const uint8_t *uuid, k_timeout_t timeout)
{
	struct bt_mesh_test_msg *msg = blocking_recv(timeout);

	if (!msg) {
		return -ETIMEDOUT;
	}

	if (len != msg->len) {
		LOG_ERR("Recv: Invalid message length (%u, expected %u)", msg->len, len);
		return -EINVAL;
	}

	if (dst != BT_MESH_ADDR_UNASSIGNED && dst != msg->ctx.recv_dst) {
		LOG_ERR("Recv: Invalid dst 0x%04x, expected 0x%04x", msg->ctx.recv_dst, dst);
		return -EINVAL;
	}

	if (BT_MESH_ADDR_IS_VIRTUAL(msg->ctx.recv_dst) &&
	    ((uuid != NULL && msg->ctx.uuid == NULL) ||
	     (uuid == NULL && msg->ctx.uuid != NULL) ||
	     memcmp(uuid, msg->ctx.uuid, 16))) {
		LOG_ERR("Recv: Label UUID mismatch for virtual address 0x%04x");
		if (uuid && msg->ctx.uuid) {
			LOG_ERR("Got: %s", bt_hex(msg->ctx.uuid, 16));
			LOG_ERR("Expected: %s", bt_hex(uuid, 16));
		}

		return -EINVAL;
	}

	k_mem_slab_free(&msg_pool, (void *)msg);

	return 0;
}

int bt_mesh_test_recv_msg(struct bt_mesh_test_msg *msg, k_timeout_t timeout)
{
	struct bt_mesh_test_msg *queued = blocking_recv(timeout);

	if (!queued) {
		return -ETIMEDOUT;
	}

	*msg = *queued;

	k_mem_slab_free(&msg_pool, (void *)queued);

	return 0;
}

int bt_mesh_test_recv_clear(void)
{
	struct bt_mesh_test_msg *queued;
	int count = 0;

	while ((queued = k_queue_get(&recv, K_NO_WAIT))) {
		k_mem_slab_free(&msg_pool, (void *)queued);
		count++;
	}

	return count;
}

struct sync_send_ctx {
	struct k_sem sem;
	int err;
};

static void tx_started(uint16_t dur, int err, void *data)
{
	struct sync_send_ctx *send_ctx = data;

	if (err) {
		LOG_ERR("Couldn't start sending (err: %d)", err);

		send_ctx->err = err;
		k_sem_give(&send_ctx->sem);

		return;
	}

	LOG_INF("Sending started");
}

static void tx_ended(int err, void *data)
{
	struct sync_send_ctx *send_ctx = data;

	send_ctx->err = err;

	if (err) {
		LOG_ERR("Send failed (%d)", err);
	} else {
		LOG_INF("Sending ended");
	}

	k_sem_give(&send_ctx->sem);
}

int bt_mesh_test_send_async(uint16_t addr, const uint8_t *uuid, size_t len,
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
	test_send_ctx.uuid = uuid;

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MSG_OP_1, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&buf, TEST_MSG_OP_1);

	if (len > BT_MESH_MODEL_OP_LEN(TEST_MSG_OP_1)) {
		net_buf_simple_add_u8(&buf, count);
	}

	/* Subtract the length of the opcode and the sequence ID */
	for (int i = 1; i < len - BT_MESH_MODEL_OP_LEN(TEST_MSG_OP_1); i++) {
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

int bt_mesh_test_send(uint16_t addr, const uint8_t *uuid, size_t len,
		      enum bt_mesh_test_send_flags flags, k_timeout_t timeout)
{
	if (K_TIMEOUT_EQ(timeout, K_NO_WAIT)) {
		return bt_mesh_test_send_async(addr, uuid, len, flags, NULL, NULL);
	}

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	int64_t uptime = k_uptime_get();
	struct sync_send_ctx send_ctx;
	int err;

	k_sem_init(&send_ctx.sem, 0, 1);
	err = bt_mesh_test_send_async(addr, uuid, len, flags, &send_cb, &send_ctx);
	if (err) {
		return err;
	}

	err = k_sem_take(&send_ctx.sem, timeout);
	if (err) {
		LOG_ERR("Send timed out");
		return err;
	}

	if (send_ctx.err) {
		return send_ctx.err;
	}

	LOG_INF("Sending completed (%lld ms)", k_uptime_delta(&uptime));

	return 0;
}

int bt_mesh_test_send_ra(uint16_t addr, uint8_t *data, size_t len,
			 const struct bt_mesh_send_cb *send_cb,
			 void *cb_data)
{
	int err;

	test_send_ctx.addr = addr;
	test_send_ctx.send_rel = 0;
	test_send_ctx.send_ttl = BT_MESH_TTL_DEFAULT;

	BT_MESH_MODEL_BUF_DEFINE(buf, TEST_MSG_OP_2, BT_MESH_TX_SDU_MAX);
	bt_mesh_model_msg_init(&buf, TEST_MSG_OP_2);

	net_buf_simple_add_mem(&buf, data, len);

	err = bt_mesh_model_send(test_model, &test_send_ctx, &buf, send_cb, cb_data);
	if (err) {
		LOG_ERR("bt_mesh_model_send failed (err: %d)", err);
		return err;
	}

	return 0;
}

void bt_mesh_test_ra_cb_setup(void (*cb)(uint8_t *, size_t))
{
	ra_cb = cb;
}

uint16_t bt_mesh_test_own_addr_get(uint16_t start_addr)
{
	return start_addr + get_device_nbr();
}

#if defined(CONFIG_BT_MESH_SAR_CFG)
void bt_mesh_test_sar_conf_set(struct bt_mesh_sar_tx *tx_set, struct bt_mesh_sar_rx *rx_set)
{
	int err;

	if (tx_set) {
		struct bt_mesh_sar_tx tx_rsp;

		err = bt_mesh_sar_cfg_cli_transmitter_set(0, cfg->addr, tx_set, &tx_rsp);
		if (err) {
			FAIL("Failed to configure SAR Transmitter state (err %d)", err);
		}
	}

	if (rx_set) {
		struct bt_mesh_sar_rx rx_rsp;

		err = bt_mesh_sar_cfg_cli_receiver_set(0, cfg->addr, rx_set, &rx_rsp);
		if (err) {
			FAIL("Failed to configure SAR Receiver state (err %d)", err);
		}
	}
}
#endif /* defined(CONFIG_BT_MESH_SAR_CFG) */
