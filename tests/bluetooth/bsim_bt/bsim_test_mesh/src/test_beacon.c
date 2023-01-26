/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include "mesh_test.h"
#include "mesh/adv.h"
#include "mesh/net.h"
#include "mesh/beacon.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"
#include "mesh/crypto.h"

#define LOG_MODULE_NAME test_beacon

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define GROUP_ADDR 0xc000
#define WAIT_TIME 60 /*seconds*/
#define MULT_NETKEYS_WAIT_TIME 350 /*seconds*/
#define BEACON_INTERVAL_WAIT_TIME 750 /*seconds*/
#define BEACON_INTERVAL 10 /*seconds*/

#define BEACON_TYPE_SECURE 0x01

static uint8_t test_net_key_secondary[16] = { 0xca, 0x11, 0xab, 0x1e };
static struct {
	uint8_t primary[16];
	uint8_t secondary[16];
} net_key_pairs[] = {
	{ "\x01\x02", "\x03\x04" },
	{ "\x11\x12", "\x13\x14" },
	{ "\x21\x22", "\x23\x24" },
	{ "\x31\x32", "\x33\x34" },
};

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

static struct k_sem observer_sem;
static struct {
	uint8_t flags;
	uint32_t iv_index;
	bool (*process_cb)(const uint8_t *net_id, void *ctx);
	void *user_ctx;
} beacon;

static void beacon_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			   struct net_buf_simple *buf)
{
	const uint8_t *net_id;
	uint8_t length;

	ASSERT_EQUAL(BT_GAP_ADV_TYPE_ADV_NONCONN_IND, adv_type);

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(BT_DATA_MESH_BEACON, net_buf_simple_pull_u8(buf));
	ASSERT_EQUAL(BEACON_TYPE_SECURE, net_buf_simple_pull_u8(buf));

	beacon.flags = net_buf_simple_pull_u8(buf);
	net_id = net_buf_simple_pull_mem(buf, 8);
	beacon.iv_index = net_buf_simple_pull_be32(buf);

	if (!beacon.process_cb || beacon.process_cb(net_id, beacon.user_ctx)) {
		k_sem_give(&observer_sem);
	}
}

/* Listens to beacons for one beacon interval (10 seconds). */
static bool wait_for_beacon(bool (*process_cb)(const uint8_t *net_id, void *ctx), void *ctx)
{
	struct bt_le_scan_param scan_param = {
			.type       = BT_HCI_LE_SCAN_PASSIVE,
			.options    = BT_LE_SCAN_OPT_NONE,
			.interval   = BT_MESH_ADV_SCAN_UNIT(1000),
			.window     = BT_MESH_ADV_SCAN_UNIT(1000)
	};
	bool received = false;
	int err;

	beacon.process_cb = process_cb;
	beacon.user_ctx = ctx;

	err = bt_le_scan_start(&scan_param, beacon_scan_cb);
	if (err && err != -EALREADY) {
		FAIL("starting scan failed (err %d)", err);
	}

	/* Listen to beacons ONLY for one beacon interval.
	 * Tests start quite often the waiting for the next beacon after
	 * transmission or receiving the previous one. If start waiting timer
	 * for BEACON_INTERVAL interval then timer expiration and receiving of
	 * the beacon happen about the same time. That is possible unstable behavior
	 * or failing some tests. To avoid this it is worth to add 1 second to
	 * waiting time (BEACON_INTERVAL + 1) to guarantee that beacon comes
	 * before timer expiration.
	 */
	err = k_sem_take(&observer_sem, K_SECONDS(BEACON_INTERVAL + 1));
	if (!err) {
		received = true;
	} else {
		LOG_WRN("Didn't receive SNB in time (err: %d)", err);
	}

	err = bt_le_scan_stop();
	if (err && err != -EALREADY) {
		FAIL("stopping scan failed (err %d)", err);
	}

	/* Sleep a little to get to the next beacon interval. Otherwise, calling this function
	 * again will catch the old beacon. This happens due to a known bug in legacy advertiser,
	 * which transmits advertisements longer than should.
	 */
	if (received && IS_ENABLED(CONFIG_BT_MESH_ADV_LEGACY)) {
		k_sleep(K_SECONDS(1));
	}

	return received;
}

static void send_beacon(struct net_buf_simple *buf)
{
	struct bt_data ad;
	int err;

	ad.type = BT_DATA_MESH_BEACON;
	ad.data = buf->data;
	ad.data_len = buf->len;

	err = bt_le_adv_start(BT_LE_ADV_NCONN, &ad, 1, NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
	}

	LOG_INF("Advertising started\n");

	k_sleep(K_MSEC(100));

	err = bt_le_adv_stop();
	if (err) {
		FAIL("Unable to stop advertising");
	}
}

static void beacon_create(struct net_buf_simple *buf, const uint8_t *net_key, uint8_t flags,
			  uint32_t iv_index)
{
	uint8_t beacon_key[16];
	uint8_t net_id[8];
	uint8_t auth[8];
	int err;

	err = bt_mesh_k3(net_key, net_id);
	if (err) {
		FAIL("Unable to generate Net ID");
	}

	err = bt_mesh_beacon_key(net_key, beacon_key);
	if (err) {
		FAIL("Unable to generate beacon key");
	}

	err = bt_mesh_beacon_auth(beacon_key, flags, net_id, iv_index, auth);
	if (err) {
		FAIL("Unable to generate auth value");
	}

	net_buf_simple_reset(buf);
	net_buf_simple_add_u8(buf, BEACON_TYPE_SECURE);
	net_buf_simple_add_u8(buf, flags);
	net_buf_simple_add_mem(buf, net_id, 8);
	net_buf_simple_add_be32(buf, iv_index);
	net_buf_simple_add_mem(buf, auth, 8);
}

/* Test reception of invalid beacons. */
static void test_tx_invalid(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	int err;

	bt_mesh_test_cfg_set(&tx_cfg, 130);
	k_sem_init(&observer_sem, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	/* Let the rx node send the first beacon. */
	k_sleep(K_SECONDS(5));

	/* Create a valid beacon with IV Update Flag set to 1 and new IV Index. */
	beacon_create(&buf, test_net_key, 0x02, 0x0001);

	/* Corrupt Flags. The node shall ignore the beacon and continue sending beacons regularly.*/
	buf.data[1] ^= 0xFF;
	send_beacon(&buf);
	buf.data[1] ^= 0xFF;
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* Corrupt Network ID. The node shall ignore the beacon and continue sending beacons
	 * regularly.
	 */
	buf.data[2] ^= 0xFF;
	send_beacon(&buf);
	buf.data[2] ^= 0xFF;
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* Corrupt IV Index. The node shall ignore the beacon and continue sending beacons
	 * regularly.
	 */
	buf.data[10] ^= 0xFF;
	send_beacon(&buf);
	buf.data[10] ^= 0xFF;
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* Corrupt Authentication value. The node shall ignore the beacon and continue sending
	 * beacons regularly.
	 */
	buf.data[14] ^= 0xFF;
	send_beacon(&buf);
	buf.data[14] ^= 0xFF;
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* Now the beacon payload is valid and it shall trigger IV Update on the node. It shall also
	 * increase the beacon interval.
	 */
	send_beacon(&buf);
	/* The beacon interval shall be changed and the node shall skip transmission of the next
	 * beacon.
	 */
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	PASS();
}

/* Test reception of invalid beacons. */
static void test_rx_invalid(void)
{
	bt_mesh_test_cfg_set(&rx_cfg, 130);
	bt_mesh_test_setup();
	bt_mesh_iv_update_test(true);

	k_sleep(K_SECONDS(10));

	for (size_t i = 0; i < 4; i++) {
		ASSERT_FALSE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
		ASSERT_EQUAL(0, bt_mesh.iv_index);

		k_sleep(K_SECONDS((BEACON_INTERVAL + 1) * 2));
	}

	/* Only the last beacon shall change IV Update state. */
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_EQUAL(1, bt_mesh.iv_index);

	PASS();
}

/* Test beacons reception with Key Refresh and IV Update on primary subnet. */
static void test_tx_kr_old_key(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	int err;

	bt_mesh_test_cfg_set(&tx_cfg, 170);
	k_sem_init(&observer_sem, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	/* Let the rx node send the first beacon. */
	k_sleep(K_SECONDS(5));

	/* The node has added a new Net Key. */

	/* Send a beacon with Key Refresh flag set to 1, but secured with the old Net Key. The
	 * beacon shall not change Key Refresh phase, but should still be processed. The beacon
	 * interval shall be increased.
	 */
	beacon_create(&buf, test_net_key, 0x01, 0x0000);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* The old Net Key can still initiate IV Index update. */
	beacon_create(&buf, test_net_key, 0x02, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with Key Refresh flag set to 1, IV Update flag set to 1, but secured with
	 * the new Net Key. The node shall set Key Refresh phase to 2. The beacon interval shall
	 * be increased.
	 */
	beacon_create(&buf, test_net_key_secondary, 0x03, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with Key Refresh flag set to 1, IV Update flag set to 0, but secured with
	 * the old Net Key. The beacon shall be rejected. The beacon interval shall not be changed.
	 */
	beacon_create(&buf, test_net_key, 0x01, 0x0001);
	send_beacon(&buf);
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Try the same with the new Net Key. Now the node shall change Key Refresh phase to 0. The
	 * beacon interval shall be increased.
	 */
	beacon_create(&buf, test_net_key_secondary, 0x02, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with IV Update flag set to 0 and secured with the old Net Key. The beacon
	 * shall be ignored. The beacon interval shall not be changed.
	 */
	beacon_create(&buf, test_net_key, 0x00, 0x0001);
	send_beacon(&buf);
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Do the same, but secure beacon with the new Net Key. Now the node shall change IV Update
	 * flag to 0. The beacon interval shall be increased.
	 */
	beacon_create(&buf, test_net_key_secondary, 0x00, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	PASS();
}

/* Test beacons reception with Key Refresh and IV Update on primary subnet. */
static void test_rx_kr_old_key(void)
{
	uint8_t phase;
	uint8_t status;
	int err;

	bt_mesh_test_cfg_set(&rx_cfg, 170);
	bt_mesh_test_setup();
	bt_mesh_iv_update_test(true);

	err = bt_mesh_cfg_cli_net_key_update(0, cfg->addr, 0, test_net_key_secondary, &status);
	if (err || status) {
		FAIL("Net Key update failed (err %d, status %u)", err, status);
	}

	static struct {
		uint8_t phase;
		bool ivu;
		uint32_t ivi;
	} test_vector[] = {
		/* Old Net Key, attempt to change Key Refresh phase to 2.  */
		{ .phase = BT_MESH_KR_PHASE_1, .ivu = false, .ivi = 0 },
		/* Old Net Key, changing IV Update state. */
		{ .phase = BT_MESH_KR_PHASE_1, .ivu = true, .ivi = 1 },
		/* New Net Key, changing Key Refresh phase. */
		{ .phase = BT_MESH_KR_PHASE_2, .ivu = true, .ivi = 1 },
		/* Old Net Key, attempt to change IV Update state. */
		{ .phase = BT_MESH_KR_PHASE_2, .ivu = true, .ivi = 1 },
		/* New Net Key, changing Key Refresh phase to 0. */
		{ .phase = BT_MESH_KR_NORMAL, .ivu = true, .ivi = 1 },
		/* Old Net Key, attempt to change IV Update state to Idle.*/
		{ .phase = BT_MESH_KR_NORMAL, .ivu = true, .ivi = 1 },
		/* Net Net Key, changing IV Update state to Idle. */
		{ .phase = BT_MESH_KR_NORMAL, .ivu = false, .ivi = 1 },
	};

	k_sleep(K_SECONDS(10));

	for (size_t i = 0; i < ARRAY_SIZE(test_vector); i++) {
		status = bt_mesh_subnet_kr_phase_get(0, &phase);
		if (status != STATUS_SUCCESS) {
			FAIL("Unable to populate Key Refresh phase (status: %d)", status);
		}

		ASSERT_EQUAL(test_vector[i].phase, phase);
		ASSERT_EQUAL(test_vector[i].ivu, atomic_test_bit(bt_mesh.flags,
								 BT_MESH_IVU_IN_PROGRESS));
		ASSERT_EQUAL(test_vector[i].ivi, bt_mesh.iv_index);

		k_sleep(K_SECONDS((BEACON_INTERVAL + 1) * 2));
	}

	PASS();
}

static bool beacon_confirm_by_subnet(const uint8_t *net_id, void *ctx)
{
	const uint8_t *expected_net_id = ctx;

	return !memcmp(expected_net_id, net_id, 8);
}

static bool beacon_confirm_all_subnets(const uint8_t *net_id, void *ctx)
{
	static uint32_t counter;
	int err;

	for (size_t i = 0; i < ARRAY_SIZE(net_key_pairs); i++) {
		uint8_t expected_net_id[8];

		err = bt_mesh_k3(net_key_pairs[i].secondary, expected_net_id);
		if (err) {
			FAIL("Unable to generate Net ID");
		}

		if (!memcmp(expected_net_id, net_id, 8)) {
			LOG_INF("Received beacon for Net Key Idx %d", (i + 1));
			counter |= 1 << i;

			ASSERT_EQUAL(0x00, beacon.flags);
			ASSERT_EQUAL(0x0000, beacon.iv_index);
		}
	}

	if (counter == BIT_MASK(ARRAY_SIZE(net_key_pairs))) {
		counter = 0;
		return true;
	} else {
		return false;
	}
}

/* Test beacons rejection with multiple Net Keys. */
static void test_tx_multiple_netkeys(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	int err;

	bt_mesh_test_cfg_set(&tx_cfg, MULT_NETKEYS_WAIT_TIME);
	k_sem_init(&observer_sem, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
		return;
	}

	LOG_INF("Bluetooth initialized");

	/* Let the rx node send the first beacon. */
	k_sleep(K_SECONDS(5));

	/* The node has added new Net Keys. */

	for (size_t i = 0; i < ARRAY_SIZE(net_key_pairs); i++) {
		uint8_t net_id_secondary[8];

		err = bt_mesh_k3(net_key_pairs[i].secondary, net_id_secondary);
		if (err) {
			FAIL("Unable to generate Net ID");
		}

		/* Send beacon with Key Refresh flag set to 1, but secured with the old Net Key.
		 * The beacon shall be processed, but shall not change Key Refresh phase.
		 */
		beacon_create(&buf, net_key_pairs[i].primary, 0x01, 0x0000);
		send_beacon(&buf);
		ASSERT_FALSE(wait_for_beacon(beacon_confirm_by_subnet, &buf.data[2]));
		ASSERT_TRUE(wait_for_beacon(beacon_confirm_by_subnet, &buf.data[2]));
		ASSERT_EQUAL(0x00, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Do the same, but secure beacon with the new Net Key. The node shall set Key
		 * Refresh phase to 2.
		 */
		beacon_create(&buf, net_key_pairs[i].secondary, 0x01, 0x0000);
		send_beacon(&buf);
		ASSERT_FALSE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x01, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Send beacon with Key Refresh flag set to 0, but secured with the old Net Key.
		 * The beacon shall be rejected. The beacon interval shall not be changed.
		 */
		beacon_create(&buf, net_key_pairs[i].primary, 0x00, 0x0000);
		send_beacon(&buf);
		ASSERT_TRUE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x01, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Do the same with the new Net Key. Now the node shall change Key Refresh phase
		 * to 0. The beacon interval shall be increased.
		 */
		beacon_create(&buf, net_key_pairs[i].secondary, 0x00, 0x0000);
		send_beacon(&buf);
		ASSERT_FALSE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x00, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);
	}

	/* Create a valid beacon secured with unknown Net Key. The node shall ignore the beacon and
	 * continue sending beacons regularly.
	 */
	uint8_t unknown_net_key[16] = { 0xde, 0xad, 0xbe, 0xef };

	beacon_create(&buf, unknown_net_key, 0x00, 0x0000);
	send_beacon(&buf);
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(beacon_confirm_all_subnets, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_confirm_all_subnets, NULL));

	PASS();
}

/* Test beacons rejection with multiple Net Keys. */
static void test_rx_multiple_netkeys(void)
{
	uint8_t phase;
	uint8_t status;
	int err;

	bt_mesh_test_cfg_set(&rx_cfg, MULT_NETKEYS_WAIT_TIME);
	bt_mesh_test_setup();
	bt_mesh_iv_update_test(true);

	/* Add new Net Keys and switch Key Refresh phase to 1 so that beacons can trigger Key
	 * Refresh procedure.
	 */
	for (size_t i = 0; i < ARRAY_SIZE(net_key_pairs); i++) {
		err = bt_mesh_cfg_cli_net_key_add(0, cfg->addr, i + 1, net_key_pairs[i].primary,
					      &status);
		if (err || status) {
			FAIL("Net Key add failed (err %d, status %u)", err, status);
		}

		err = bt_mesh_cfg_cli_net_key_update(0, cfg->addr, i + 1,
						     net_key_pairs[i].secondary, &status);
		if (err || status) {
			FAIL("Net Key update failed (err %d, status %u)", err, status);
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(net_key_pairs); i++) {
		/* Tx device shall change Key Refresh phase to 2. */
		k_sleep(K_SECONDS(40));

		status = bt_mesh_subnet_kr_phase_get(i + 1, &phase);
		if (status != STATUS_SUCCESS) {
			FAIL("Unable to populate Key Refresh phase (status: %d)", status);
		}

		ASSERT_EQUAL(BT_MESH_KR_PHASE_2, phase);

		/* Tx device shall change Key Refresh phase to 0. */
		k_sleep(K_SECONDS(40));

		status = bt_mesh_subnet_kr_phase_get(i + 1, &phase);
		if (status != STATUS_SUCCESS) {
			FAIL("Unable to populate Key Refresh phase (status: %d)", status);
		}

		ASSERT_EQUAL(BT_MESH_KR_NORMAL, phase);
	}

	PASS();

}

static struct k_work_delayable beacon_timer;

static void secure_beacon_send(struct k_work *work)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	beacon_create(&buf, test_net_key, 0, 0);
	send_beacon(&buf);
	/**
	 * Sending SNB(secure network beacon) faster to guarantee
	 * at least one beacon is received by tx node in 10s period.
	 */
	k_work_schedule(&beacon_timer, K_SECONDS(2));
}

static void test_tx_secure_beacon_interval(void)
{
	bt_mesh_test_cfg_set(&tx_cfg, BEACON_INTERVAL_WAIT_TIME);
	k_sleep(K_SECONDS(2));
	bt_mesh_test_setup();
	PASS();
}

static void test_rx_secure_beacon_interval(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	int err;
	int64_t beacon_recv_time;
	int64_t delta;

	bt_mesh_test_cfg_set(&rx_cfg, BEACON_INTERVAL_WAIT_TIME);
	k_sem_init(&observer_sem, 0, 1);
	k_work_init_delayable(&beacon_timer, secure_beacon_send);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
	}

	beacon_create(&buf, test_net_key, 0, 0);
	k_sleep(K_SECONDS(5));
	/*wait provisioned tx node to send the first beacon*/
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	k_sleep(K_SECONDS(2));

	/**
	 * Sending 2 SNB 20ms apart by only sending for even values of loop variable.
	 * And verify that tx node adapts to 20s SNB interval after sending enough
	 * beacons in for loop.
	 */
	for (size_t i = 1; i < 5; i++) {
		if (i % 2) {
			send_beacon(&buf);
			ASSERT_FALSE(wait_for_beacon(NULL, NULL));
		} else {
			ASSERT_TRUE(wait_for_beacon(NULL, NULL));
		}
	}

	/**
	 * Verify that tx node keeps the 20s SNB interval until adapts itself and
	 * sends SNB in 10s again.
	 */
	ASSERT_FALSE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(NULL, NULL));
	beacon_recv_time = k_uptime_get();
	/* Start sending SNB */
	k_work_schedule(&beacon_timer, K_NO_WAIT);

	/**
	 * Send SNB so that the tx node stays silent and eventually sends
	 * an SNB after 600s, which is the maximum time for SNB interval.
	 * Sending beacon with 2sec interval.
	 */
	delta = 0;
	for (size_t i = 0; i < 60; i++) {
		if (wait_for_beacon(NULL, NULL)) {
			delta = k_uptime_delta(&beacon_recv_time);
			break;
		}
	}

	ASSERT_TRUE(delta >= (600 * MSEC_PER_SEC));
	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "beacon_" #role "_" #name,          \
		.test_descr = description,                     \
		.test_pre_init_f = test_##role##_init,         \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_beacon[] = {
	TEST_CASE(tx, on_iv_update,   "Beacon: send on IV update"),
	TEST_CASE(tx, on_key_refresh,  "Beacon: send on key refresh"),
	TEST_CASE(tx, invalid, "Beacon: send invalid beacon"),
	TEST_CASE(tx, kr_old_key, "Beacon: send old Net Key"),
	TEST_CASE(tx, multiple_netkeys, "Beacon: multiple Net Keys"),
	TEST_CASE(tx, secure_beacon_interval, "Beacon: send secure beacons"),

	TEST_CASE(rx, on_iv_update,   "Beacon: receive with IV update flag"),
	TEST_CASE(rx, on_key_refresh,  "Beacon: receive with key refresh flag"),
	TEST_CASE(rx, invalid, "Beacon: receive invalid beacon"),
	TEST_CASE(rx, kr_old_key, "Beacon: receive old Net Key"),
	TEST_CASE(rx, multiple_netkeys, "Beacon: multiple Net Keys"),
	TEST_CASE(rx, secure_beacon_interval, "Beacon: receive and send secure beacons"),
	BSTEST_END_MARKER
};

struct bst_test_list *test_beacon_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_beacon);
	return tests;
}
