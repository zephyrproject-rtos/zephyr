/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include "mesh_test.h"
#include "mesh/mesh.h"
#include "mesh/net.h"
#include "mesh/rpl.h"
#include "mesh/transport.h"

#define LOG_MODULE_NAME test_rpc

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/
#define TEST_DATA_WAITING_TIME 5 /* seconds */
#define TEST_DATA_SIZE 20

static const struct bt_mesh_test_cfg tx_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg rx_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};

static uint8_t test_data[TEST_DATA_SIZE];
static uint8_t rx_cnt;
static bool is_tx_succeeded;

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
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
		is_tx_succeeded = false;
		LOG_INF("Sending failed (%d)", err);
	} else {
		is_tx_succeeded = true;
		LOG_INF("Sending succeeded");
	}

	k_sem_give(sem);
}

static void rx_ended(uint8_t *data, size_t len)
{
	memset(test_data, rx_cnt++, sizeof(test_data));

	if (memcmp(test_data, data, len)) {
		FAIL("Unexpected rx data");
	}

	LOG_INF("Receiving succeeded");
}

static void tx_sar_conf(void)
{
#ifdef CONFIG_BT_MESH_V1d1
	/* Reconfigure SAR Transmitter state so that the transport layer doesn't
	 * retransmit.
	 */
	struct bt_mesh_sar_tx tx_set = {
		.seg_int_step = CONFIG_BT_MESH_SAR_TX_SEG_INT_STEP,
		.unicast_retrans_count = 0,
		.unicast_retrans_without_prog_count = 0,
		.unicast_retrans_int_step = CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_STEP,
		.unicast_retrans_int_inc = CONFIG_BT_MESH_SAR_TX_UNICAST_RETRANS_INT_INC,
		.multicast_retrans_count = CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_COUNT,
		.multicast_retrans_int = CONFIG_BT_MESH_SAR_TX_MULTICAST_RETRANS_INT,
	};

#if defined(CONFIG_BT_MESH_SAR_CFG)
	bt_mesh_test_sar_conf_set(&tx_set, NULL);
#else
	bt_mesh.sar_tx = tx_set;
#endif
#endif
}

static void rx_sar_conf(void)
{
#ifdef CONFIG_BT_MESH_V1d1
	/* Reconfigure SAR Receiver state so that the transport layer does
	 * generate Segmented Acks as rarely as possible.
	 */
	struct bt_mesh_sar_rx rx_set = {
		.seg_thresh = 0x1f,
		.ack_delay_inc = 0x7,
		.discard_timeout = CONFIG_BT_MESH_SAR_RX_DISCARD_TIMEOUT,
		.rx_seg_int_step = 0xf,
		.ack_retrans_count = CONFIG_BT_MESH_SAR_RX_ACK_RETRANS_COUNT,
	};

#if defined(CONFIG_BT_MESH_SAR_CFG)
	bt_mesh_test_sar_conf_set(NULL, &rx_set);
#else
	bt_mesh.sar_rx = rx_set;
#endif
#endif
}

static void test_tx_immediate_replay_attack(void)
{
	bt_mesh_test_setup();
	tx_sar_conf();

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	uint32_t seq = bt_mesh.seq;

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = false;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(is_tx_succeeded);
	}

	bt_mesh.seq = seq;

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = true;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(!is_tx_succeeded);
	}

	PASS();
}

static void test_rx_immediate_replay_attack(void)
{
	bt_mesh_test_setup();
	rx_sar_conf();
	bt_mesh_test_ra_cb_setup(rx_ended);

	k_sleep(K_SECONDS(6 * TEST_DATA_WAITING_TIME));

	ASSERT_TRUE_MSG(rx_cnt == 3, "Device didn't receive expected data\n");

	PASS();
}

static void test_tx_power_replay_attack(void)
{
	bt_mesh_test_setup();
	tx_sar_conf();

	static const struct bt_mesh_send_cb send_cb = {
		.start = tx_started,
		.end = tx_ended,
	};
	struct k_sem sem;

	k_sem_init(&sem, 0, 1);

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = true;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(!is_tx_succeeded);
	}

	for (int i = 0; i < 3; i++) {
		is_tx_succeeded = false;

		memset(test_data, i, sizeof(test_data));
		ASSERT_OK(bt_mesh_test_send_ra(rx_cfg.addr, test_data,
			sizeof(test_data), &send_cb, &sem));

		if (k_sem_take(&sem, K_SECONDS(TEST_DATA_WAITING_TIME))) {
			LOG_ERR("Send timed out");
		}

		ASSERT_TRUE(is_tx_succeeded);
	}

	PASS();
}

static void test_rx_power_replay_attack(void)
{
	bt_mesh_test_setup();
	rx_sar_conf();
	bt_mesh_test_ra_cb_setup(rx_ended);

	k_sleep(K_SECONDS(6 * TEST_DATA_WAITING_TIME));

	ASSERT_TRUE_MSG(rx_cnt == 3, "Device didn't receive expected data\n");

	PASS();
}

static void send_end_cb(int err, void *cb_data)
{
	struct k_sem *sem = cb_data;

	ASSERT_EQUAL(err, 0);
	k_sem_give(sem);
}

static bool msg_send(uint16_t src, uint16_t dst)
{
	static struct bt_mesh_send_cb cb = {
		.end = send_end_cb,
	};
	struct bt_mesh_msg_ctx ctx = {
		.net_idx = 0,
		.app_idx = 0,
		.addr = dst,
		.send_rel = false,
		.send_ttl = BT_MESH_TTL_DEFAULT,
	};
	struct bt_mesh_net_tx tx = {
		.ctx = &ctx,
		.src = src,
	};
	struct k_sem sem;
	int err;

	k_sem_init(&sem, 0, 1);
	BT_MESH_MODEL_BUF_DEFINE(msg, TEST_MSG_OP_1, 0);

	bt_mesh_model_msg_init(&msg, TEST_MSG_OP_1);

	err = bt_mesh_trans_send(&tx, &msg, &cb, &sem);
	if (err) {
		LOG_ERR("Failed to send message (err %d)", err);
		return false;
	}

	err = k_sem_take(&sem, K_SECONDS(10));
	if (err) {
		LOG_ERR("Send timed out (err %d)", err);
		return false;
	}

	return true;
}

static bool msg_recv(uint16_t expected_addr)
{
	struct bt_mesh_test_msg msg;
	int err;

	err = bt_mesh_test_recv_msg(&msg, K_SECONDS(10));
	if (err) {
		LOG_ERR("Failed to receive message from %u (err %d)", expected_addr, err);
		return false;
	}

	LOG_DBG("Received msg from %u", msg.ctx.addr);
	ASSERT_EQUAL(expected_addr, msg.ctx.addr);

	return true;
}

static bool ivi_update_toggle(void)
{
	bool res;

	bt_mesh_iv_update_test(true);
	res = bt_mesh_iv_update();
	bt_mesh_iv_update_test(false);

	return res;
}

/* 1 second delays have been added to prevent interfering tail of
 * the previous rx transaction with the beginning of the new tx transaction.
 */
static void test_rx_rpl_frag(void)
{
	bt_mesh_test_setup();

	k_sleep(K_SECONDS(10));

	/* Wait 3 messages from different sources. */
	for (int i = 0; i < 3; i++) {
		ASSERT_TRUE(msg_recv(100 + i));
	}

	k_sleep(K_SECONDS(1));

	/* Ask tx node to proceed to next test step. */
	ASSERT_TRUE(msg_send(rx_cfg.addr, tx_cfg.addr));

	/* Start IVI Update. This will set old_iv for all entries in RPL to 1. */
	ASSERT_TRUE(ivi_update_toggle());

	/* Receive messages from even nodes with new IVI. RPL entry with odd address will stay
	 * with old IVI.
	 */
	ASSERT_TRUE(msg_recv(100));
	ASSERT_TRUE(msg_recv(102));

	k_sleep(K_SECONDS(1));

	/* Ask tx node to proceed to next test step. */
	ASSERT_TRUE(msg_send(rx_cfg.addr, tx_cfg.addr));

	/* Complete IVI Update. */
	ASSERT_FALSE(ivi_update_toggle());

	/* Bump SeqNum in RPL for even addresses. */
	ASSERT_TRUE(msg_recv(100));
	ASSERT_TRUE(msg_recv(102));

	k_sleep(K_SECONDS(1));

	/* Start IVI Update again. */
	/* RPL entry with odd address should be removed causing fragmentation in RPL. old_iv flag
	 * for even entries will be set to 1.
	 */
	ASSERT_TRUE(ivi_update_toggle());

	/* Ask tx node to proceed to next test step. */
	ASSERT_TRUE(msg_send(rx_cfg.addr, tx_cfg.addr));

	/* Complete IVI Update. */
	ASSERT_FALSE(ivi_update_toggle());

	/* Odd address entry should have been removed keeping even addresses accessible. */
	struct bt_mesh_rpl *rpl = NULL;
	struct bt_mesh_net_rx rx = {
		.old_iv = 1,
		.seq = 0,
		.ctx.addr = 100,
		.local_match = 1,
	};
	ASSERT_TRUE(bt_mesh_rpl_check(&rx, &rpl));
	rx.ctx.addr = 101;
	ASSERT_FALSE(bt_mesh_rpl_check(&rx, &rpl));
	rx.ctx.addr = 102;
	ASSERT_TRUE(bt_mesh_rpl_check(&rx, &rpl));

	/* Let the settings store RPL. */
	k_sleep(K_SECONDS(CONFIG_BT_MESH_RPL_STORE_TIMEOUT));

	PASS();
}

/* 1 second delays have been added to prevent interfering tail of
 * the previous rx transaction with the beginning of the new tx transaction.
 */
static void test_tx_rpl_frag(void)
{
	bt_mesh_test_setup();

	k_sleep(K_SECONDS(10));

	/* Send message for 3 different addresses. */
	for (size_t i = 0; i < 3; i++) {
		ASSERT_TRUE(msg_send(100 + i, rx_cfg.addr));
	}

	/* Wait for the rx node. */
	ASSERT_TRUE(msg_recv(rx_cfg.addr));

	k_sleep(K_SECONDS(1));

	/* Start IVI Update. */
	ASSERT_TRUE(ivi_update_toggle());

	/* Send msg from elem 1 and 3 with new IVI. 2nd elem should have old IVI. */
	ASSERT_TRUE(msg_send(100, rx_cfg.addr));
	ASSERT_TRUE(msg_send(102, rx_cfg.addr));

	/* Wait for the rx node. */
	ASSERT_TRUE(msg_recv(rx_cfg.addr));

	k_sleep(K_SECONDS(1));

	/* Complete IVI Update. */
	ASSERT_FALSE(ivi_update_toggle());

	/* Send message from even addresses with new IVI keeping odd address with old IVI. */
	ASSERT_TRUE(msg_send(100, rx_cfg.addr));
	ASSERT_TRUE(msg_send(102, rx_cfg.addr));

	/* Start IVI Update again to be in sync with rx node. */
	ASSERT_TRUE(ivi_update_toggle());

	/* Wait for rx node. */
	ASSERT_TRUE(msg_recv(rx_cfg.addr));

	/* Complete IVI Update. */
	ASSERT_FALSE(ivi_update_toggle());

	PASS();
}

static void test_rx_reboot_after_defrag(void)
{
	bt_mesh_test_setup();

	/* Test that RPL entries are restored correctly after defrag and reboot. */
	struct bt_mesh_rpl *rpl = NULL;
	struct bt_mesh_net_rx rx = {
		.old_iv = 1,
		.seq = 0,
		.ctx.addr = 100,
		.local_match = 1,
	};
	ASSERT_TRUE(bt_mesh_rpl_check(&rx, &rpl));
	rx.ctx.addr = 101;
	ASSERT_FALSE(bt_mesh_rpl_check(&rx, &rpl));
	rx.ctx.addr = 102;
	ASSERT_TRUE(bt_mesh_rpl_check(&rx, &rpl));

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "rpc_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_post_init_f = test_##role##_init,        \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_rpc[] = {
	TEST_CASE(tx, immediate_replay_attack, "RPC: perform replay attack immediately"),
	TEST_CASE(tx, power_replay_attack,     "RPC: perform replay attack after power cycle"),
	TEST_CASE(tx, rpl_frag, "RPC: Send messages after double IVI Update"),

	TEST_CASE(rx, immediate_replay_attack, "RPC: device under immediate attack"),
	TEST_CASE(rx, power_replay_attack,     "RPC: device under power cycle reply attack"),
	TEST_CASE(rx, rpl_frag, "RPC: Test RPL fragmentation after double IVI Update"),
	TEST_CASE(rx, reboot_after_defrag, "RPC: Test PRL after defrag and reboot"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_rpc_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_rpc);
	return tests;
}
