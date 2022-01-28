/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <kernel.h>
#include "mesh_test.h"
#include "argparse.h"
#include "mesh/net.h"
#include "mesh/beacon.h"
#include "mesh/crypto.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_beacon

#include <logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define GROUP_ADDR 0xc000
#define WAIT_TIME 60 /*seconds*/
#define BEACON_INTERVAL  10 /*seconds*/
#define BEACON_OBSERVE_INTERVAL 60 /*seconds*/
#define EXPECTED_BEACON_CNT ((BEACON_OBSERVE_INTERVAL/BEACON_INTERVAL)-1)
#define MIN_BEACON_INTERVAL 10000 /*millisecods*/


extern enum bst_result_t bst_result;

static const struct bt_mesh_test_cfg tx_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};
static const struct bt_mesh_test_cfg rx_cfg = {
	.addr = 0x0002,
	.dev_key = { 0x02 },
};

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
}

static void ivu_log(void)
{
	LOG_DBG("ivi: %i", bt_mesh.iv_index);
	LOG_DBG("flags:");

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR)) {
		LOG_DBG("IVU initiator");
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS)) {
		LOG_DBG("IVU in progress");
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_PENDING)) {
		LOG_DBG("IVU pending");
	}

	if (atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST)) {
		LOG_DBG("IVU in test mode");
	}
}

static void test_tx_on_iv_update(void)
{
	bt_mesh_test_setup();
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_PENDING));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));
	ASSERT_TRUE(bt_mesh.iv_index == 0);

	/* shift beaconing time line to avoid boundary cases. */
	k_sleep(K_SECONDS(1));

	bt_mesh_iv_update_test(true);
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));

	ASSERT_TRUE(bt_mesh_iv_update());
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 1);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	ASSERT_TRUE(!bt_mesh_iv_update());
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 1);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	ASSERT_TRUE(bt_mesh_iv_update());
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 2);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	PASS();
}

static void test_rx_on_iv_update(void)
{
	bt_mesh_test_setup();
	/* disable beaconing from Rx device to prevent
	 * the time line adaptation due to observation algorithm.
	 */
	bt_mesh_beacon_disable();
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_PENDING));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));
	ASSERT_TRUE(bt_mesh.iv_index == 0);

	/* shift beaconing time line to avoid boundary cases. */
	k_sleep(K_SECONDS(1));

	bt_mesh_iv_update_test(true);
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));
	ivu_log();

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 1);
	ivu_log();

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 1);
	ivu_log();

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 2);
	ivu_log();

	PASS();
}

static void test_tx_on_key_refresh(void)
{
	const uint8_t new_key[16] = {0x01};
	uint8_t phase;
	uint8_t status;

	bt_mesh_test_setup();

	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_NORMAL);

	/* shift beaconing time line to avoid boundary cases. */
	k_sleep(K_SECONDS(1));

	status = bt_mesh_subnet_update(BT_MESH_KEY_PRIMARY, new_key);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_1);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	phase = BT_MESH_KR_PHASE_2;
	status = bt_mesh_subnet_kr_phase_set(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_2);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	phase = BT_MESH_KR_PHASE_3;
	status = bt_mesh_subnet_kr_phase_set(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_NORMAL);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	PASS();
}

static void test_rx_on_key_refresh(void)
{
	const uint8_t new_key[16] = {0x01};
	uint8_t phase;
	uint8_t status;

	bt_mesh_test_setup();
	/* disable beaconing from Rx device to prevent
	 * the time line adaptation due to observation algorithm.
	 */
	bt_mesh_beacon_disable();

	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_NORMAL);

	/* shift beaconing time line to avoid boundary cases. */
	k_sleep(K_SECONDS(1));

	status = bt_mesh_subnet_update(BT_MESH_KEY_PRIMARY, new_key);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_1);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_1);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_2);

	k_sleep(K_SECONDS(BEACON_INTERVAL));

	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_NORMAL);

	PASS();
}

static void test_tx_beacon_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, BEACON_OBSERVE_INTERVAL);
}

static void test_rx_beacon_init(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, BEACON_OBSERVE_INTERVAL);
}

static void test_tx_beacon_secure(void)
{
	uint32_t delay_time;
	uint32_t sent_beacons;
	uint32_t sent_beacon_time;
	struct bt_mesh_subnet *sub;
	
	/* shift beaconing time line to avoid boundary cases. */
	delay_time = get_device_nbr() * 100;
	k_sleep(K_MSEC(delay_time));

	bt_mesh_test_setup();

	sub = bt_mesh_subnet_find(NULL, NULL);

	sent_beacons = 0;
	sent_beacon_time = 0;
	while (true) {
		k_sleep(K_SECONDS(10));
		if (sent_beacon_time != sub->beacon_sent) {
			sent_beacon_time = sub->beacon_sent;
			sent_beacons++;
		}

		if (sent_beacons >= 1) { /*sending out at least 1 beacon is enough*/
			break;
		}
	}

	PASS();
}

static struct k_sem observer_sem;
static uint32_t obs_min_interval; /*minimum time measured between two consecutive beacons in milliseconds*/
static uint32_t last_beacon_time; /*timestamp for last received valid secure beacon in milliseconds*/
static uint32_t total_beacons; /*total number of beacons received*/

static void scan_recv_cb(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	struct bt_mesh_subnet *sub;
	uint8_t length;
	uint8_t payload[13];
	uint64_t *sub_auth;
	uint64_t auth;
	uint8_t mac[16];
	uint32_t now;
	uint32_t interval;

	now = k_uptime_get_32();
	interval = now - last_beacon_time;

	ASSERT_EQUAL(BT_GAP_ADV_TYPE_ADV_NONCONN_IND, info->adv_type);

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(BT_DATA_MESH_BEACON, net_buf_simple_pull_u8(buf));
	ASSERT_EQUAL(0x01, net_buf_simple_pull_u8(buf));

	sub = bt_mesh_subnet_find(NULL, NULL);

	memcpy(payload, buf->data, sizeof(payload));
	net_buf_simple_pull(buf, 13);

	if (bt_mesh_aes_cmac_one(sub->keys->beacon, payload, sizeof(payload), mac)) {
		return;
	}

	memcpy(&auth, mac, sizeof(auth));
	sub_auth = (uint64_t *)sub->auth;

	ASSERT_EQUAL(auth, *sub_auth);

	last_beacon_time = now;

	if (interval < obs_min_interval) {
		obs_min_interval = interval;
	}

	total_beacons++;
	if (total_beacons >= (EXPECTED_BEACON_CNT)) {
		k_sem_give(&observer_sem);
	}
}

static void scan_timeout_cb(void)
{
	return;
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv_cb,
	.timeout = scan_timeout_cb,
};

static void test_rx_beacon_secure(void)
{
	k_sem_init(&observer_sem, 0, 1);
	/*initial estimated interval value*/
	obs_min_interval = 10000;

	bt_mesh_test_setup();
	/* scanner will not send secure beacons */
	bt_mesh_beacon_disable();
	bt_le_scan_cb_register(&scan_callbacks);

	ASSERT_TRUE(!k_sem_take(&observer_sem, K_SECONDS(BEACON_OBSERVE_INTERVAL)));
	/*minimum time between two consecutive secure beacon shouldn't be less then 10000ms (10s)*/
	ASSERT_TRUE(obs_min_interval>=MIN_BEACON_INTERVAL);
	PASS();
}


#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "beacon_" #role "_" #name,          \
		.test_descr = description,                     \
		.test_post_init_f = test_##role##_init,        \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_beacon[] = {
	TEST_CASE(tx, on_iv_update,   "Beacon: send on IV update"),
	TEST_CASE(tx, on_key_refresh,  "Beacon: send on key refresh"),

	TEST_CASE(rx, on_iv_update,   "Beacon: receive with IV update flag"),
	TEST_CASE(rx, on_key_refresh,  "Beacon: receive with key refresh flag"),

	TEST_CASE(tx_beacon, secure, "Beacon: send secure mesh beacons"),
	TEST_CASE(rx_beacon, secure, "Beacon: receive secure mesh beacons"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_beacon_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_beacon);
	return tests;
}
