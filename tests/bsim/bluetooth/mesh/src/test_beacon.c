/*
 * Copyright (c) 2021 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/beacon.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"
#include "mesh/crypto.h"
#include "argparse.h"
#include "mesh/proxy_cli.h"
#include "mesh/proxy.h"

#define LOG_MODULE_NAME test_beacon

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define GROUP_ADDR 0xc000
#define WAIT_TIME 60 /*seconds*/
#define MULT_NETKEYS_WAIT_TIME 350 /*seconds*/
#define BEACON_INTERVAL_WAIT_TIME 750 /*seconds*/
#define BEACON_INTERVAL 10 /*seconds*/

#define BEACON_TYPE_SECURE 0x01
#define BEACON_TYPE_PRIVATE 0x02

static uint8_t test_net_key_2[16] = { 0xca, 0x11, 0xab, 0x1e };
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

typedef void (*snb_cb)(const struct bt_mesh_snb *snb);

static snb_cb snb_cb_ptr;
static struct k_sem beacon_sem;

static void snb_received(const struct bt_mesh_snb *snb)
{
	if (snb_cb_ptr) {
		snb_cb_ptr(snb);
	}
}

BT_MESH_BEACON_CB_DEFINE(snb) = {
	.snb_received = snb_received,
};

/* Setting for scanner defining what beacon is expected next, SNB as default */
static uint8_t expected_beacon = BEACON_TYPE_SECURE;
static struct bt_mesh_cfg_cli cfg_cli;

static struct bt_mesh_priv_beacon_cli priv_beacon_cli;

static const struct bt_mesh_comp prb_comp = {
	.elem =
		(const struct bt_mesh_elem[]){
			BT_MESH_ELEM(1,
				     MODEL_LIST(BT_MESH_MODEL_CFG_SRV,
						BT_MESH_MODEL_CFG_CLI(&cfg_cli),
						BT_MESH_MODEL_PRIV_BEACON_SRV,
						BT_MESH_MODEL_PRIV_BEACON_CLI(&priv_beacon_cli)),
				     BT_MESH_MODEL_NONE),
		},
	.elem_count = 1,
};

static struct bt_mesh_prov prov;
static uint8_t net_key[16] = { 1, 2, 3 };
const uint8_t app_key[16] = { 4, 5, 6 };
static uint8_t net_key_new[16] = { 7, 8, 9 };

static uint8_t last_random[13];

static bt_addr_le_t last_beacon_adv_addr;

static struct bt_mesh_key priv_beacon_key;

static int random_interval;

static void test_args_parse(int argc, char *argv[])
{
	bs_args_struct_t args_struct[] = {
		{
			.dest = &random_interval,
			.type = 'i',
			.name = "{Random interval}",
			.option = "rand-int",
			.descript = "Random interval to be set for Private Beacon"
		},
	};

	bs_args_parse_all_cmd_line(argc, argv, args_struct);
}

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

static void tx_on_iv_update_test(void)
{
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

static void test_tx_on_iv_update(void)
{
	bt_mesh_test_setup();
	tx_on_iv_update_test();
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

static void tx_on_key_refresh_test(void)
{
	const uint8_t new_key[16] = {0x01};
	uint8_t phase;
	uint8_t status;

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

static void test_tx_on_key_refresh(void)
{
	bt_mesh_test_setup();
	tx_on_key_refresh_test();
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
	uint8_t random[13];
	uint64_t pp_hash;
	uint64_t pp_random;
	uint64_t net_id;
	bt_addr_le_t adv_addr;
	bool (*process_cb)(const uint8_t *net_id, void *ctx);
	void *user_ctx;
} beacon;

static void beacon_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			   struct net_buf_simple *buf)
{
	const uint8_t *net_id;
	uint8_t ad_data_type, beacon_type, length;

	ASSERT_EQUAL(BT_GAP_ADV_TYPE_ADV_NONCONN_IND, adv_type);

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ad_data_type = net_buf_simple_pull_u8(buf);

	if (ad_data_type != BT_DATA_MESH_BEACON) {
		return;
	}

	beacon_type = net_buf_simple_pull_u8(buf);
	if (expected_beacon == BEACON_TYPE_SECURE) {
		ASSERT_EQUAL(expected_beacon, beacon_type);
		beacon.flags = net_buf_simple_pull_u8(buf);
		net_id = net_buf_simple_pull_mem(buf, 8);
		beacon.iv_index = net_buf_simple_pull_be32(buf);
	}
	else if (expected_beacon == BEACON_TYPE_PRIVATE) {
		uint8_t private_beacon_data[5];

		ASSERT_EQUAL(expected_beacon, beacon_type);
		memcpy(beacon.random, buf->data, 13);
		bt_addr_le_copy(&beacon.adv_addr, addr);

		bt_mesh_beacon_decrypt(&priv_beacon_key, &buf->data[0], &buf->data[13],
						 &buf->data[20], private_beacon_data);
		beacon.flags = private_beacon_data[0];
		beacon.iv_index = sys_get_be32(&private_beacon_data[1]);
	}

	if (!beacon.process_cb || beacon.process_cb(net_id, beacon.user_ctx)) {
		k_sem_give(&observer_sem);
	}
}

/* Listens to beacons */
static bool wait_for_beacon(bt_le_scan_cb_t scan_cb, uint16_t wait,
			    bool (*process_cb)(const uint8_t *net_id, void *ctx), void *ctx)
{
	beacon.process_cb = process_cb;
	beacon.user_ctx = ctx;

	/* Listen to beacons ONLY for one beacon interval.
	 * Tests start quite often the waiting for the next beacon after
	 * transmission or receiving the previous one. If start waiting timer
	 * for BEACON_INTERVAL interval then timer expiration and receiving of
	 * the beacon happen about the same time. That is possible unstable behavior
	 * or failing some tests. To avoid this it is worth to add 1 second to
	 * waiting time (BEACON_INTERVAL + 1) to guarantee that beacon comes
	 * before timer expiration.
	 */
	bool received = !bt_mesh_test_wait_for_packet(scan_cb, &observer_sem, wait);

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

static void beacon_create(struct net_buf_simple *buf, const uint8_t net_key[16], uint8_t flags,
			  uint32_t iv_index)
{
	struct bt_mesh_key beacon_key;
	uint8_t net_id[8];
	uint8_t auth[8];
	int err;

	err = bt_mesh_k3(net_key, net_id);
	if (err) {
		FAIL("Unable to generate Net ID");
	}

	err = bt_mesh_beacon_key(net_key, &beacon_key);
	if (err) {
		FAIL("Unable to generate beacon key");
	}

	err = bt_mesh_beacon_auth(&beacon_key, flags, net_id, iv_index, auth);
	if (err) {
		FAIL("Unable to generate auth value");
	}

	err = bt_mesh_key_destroy(&beacon_key);
	if (err) {
		FAIL("Unable to destroy beacon key");
	}

	net_buf_simple_reset(buf);
	net_buf_simple_add_u8(buf, BEACON_TYPE_SECURE);
	net_buf_simple_add_u8(buf, flags);
	net_buf_simple_add_mem(buf, net_id, 8);
	net_buf_simple_add_be32(buf, iv_index);
	net_buf_simple_add_mem(buf, auth, 8);
}

/* Test reception of invalid beacons. */
static void corrupted_beacon_test(const uint8_t *offsets,
				  k_ssize_t field_count,
				  struct net_buf_simple *buf)
{
	/* Send corrupted beacons  */
	for (int i = 0; i < field_count; i++) {
		buf->data[offsets[i]] ^= 0xFF;
		send_beacon(buf);
		buf->data[offsets[i]] ^= 0xFF;
		/* Ensure that interval is not affected. */
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
		ASSERT_EQUAL(0x00, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);
	}

	/* Now the beacon payload is valid and it shall trigger IV Update on the node. It shall also
	 * increase the beacon interval. We delay the outgoing beacon for a couple of seconds to
	 * avoid near perfect syncing with the beacon interval cycle of the device we just received
	 * a beacon from.
	 */
	k_sleep(K_SECONDS(3));
	send_beacon(buf);

	/* The beacon interval shall be changed and the node shall skip transmission of the next
	 * beacon.
	 */
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);
}

static void test_tx_invalid(void)
{
	NET_BUF_SIMPLE_DEFINE(buf, 22);
	/* Offsets of data to be corrupted: Flags, Network ID, IV Index, Authentication value */
	uint8_t fields_offsets[4] = {1, 2, 10, 14};
	int err;

	bt_mesh_test_cfg_set(&tx_cfg, 130);
	bt_mesh_crypto_init();
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

	corrupted_beacon_test(fields_offsets, ARRAY_SIZE(fields_offsets), &buf);

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
	bt_mesh_crypto_init();
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
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x00, beacon.flags);
	ASSERT_EQUAL(0x0000, beacon.iv_index);

	/* The old Net Key can still initiate IV Index update. */
	beacon_create(&buf, test_net_key, 0x02, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with Key Refresh flag set to 1, IV Update flag set to 1, but secured with
	 * the new Net Key. The node shall set Key Refresh phase to 2. The beacon interval shall
	 * be increased.
	 */
	beacon_create(&buf, test_net_key_2, 0x03, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with Key Refresh flag set to 1, IV Update flag set to 0, but secured with
	 * the old Net Key. The beacon shall be rejected. The beacon interval shall not be changed.
	 */
	beacon_create(&buf, test_net_key, 0x01, 0x0001);
	send_beacon(&buf);
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Try the same with the new Net Key. Now the node shall change Key Refresh phase to 0. The
	 * beacon interval shall be increased.
	 */
	beacon_create(&buf, test_net_key_2, 0x02, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Send beacon with IV Update flag set to 0 and secured with the old Net Key. The beacon
	 * shall be ignored. The beacon interval shall not be changed.
	 */
	beacon_create(&buf, test_net_key, 0x00, 0x0001);
	send_beacon(&buf);
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* Do the same, but secure beacon with the new Net Key. Now the node shall change IV Update
	 * flag to 0. The beacon interval shall be increased.
	 */
	beacon_create(&buf, test_net_key_2, 0x00, 0x0001);
	send_beacon(&buf);
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
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

	err = bt_mesh_cfg_cli_net_key_update(0, cfg->addr, 0, test_net_key_2, &status);
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

	k_sleep(K_SECONDS(8));

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
	bt_mesh_crypto_init();
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
		ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					     beacon_confirm_by_subnet, &buf.data[2]));
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					    beacon_confirm_by_subnet, &buf.data[2]));
		ASSERT_EQUAL(0x00, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Wait for end of sending all beacons from the rx node before sending beacon back
		 * to prevent beacon collision.
		 */
		k_sleep(K_MSEC(500));

		/* Do the same, but secure beacon with the new Net Key. The node shall set Key
		 * Refresh phase to 2.
		 */
		beacon_create(&buf, net_key_pairs[i].secondary, 0x01, 0x0000);
		send_beacon(&buf);
		ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					     beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					    beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x01, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Wait for end of sending all beacons from the rx node before sending beacon back
		 * to prevent beacon collision.
		 */
		k_sleep(K_MSEC(500));

		/* Send beacon with Key Refresh flag set to 0, but secured with the old Net Key.
		 * The beacon shall be rejected. The beacon interval shall not be changed.
		 */
		beacon_create(&buf, net_key_pairs[i].primary, 0x00, 0x0000);
		send_beacon(&buf);
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					    beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					    beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x01, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Wait for end of sending all beacons from the rx node before sending beacon back
		 * to prevent beacon collision.
		 */
		k_sleep(K_MSEC(500));

		/* Do the same with the new Net Key. Now the node shall change Key Refresh phase
		 * to 0. The beacon interval shall be increased.
		 */
		beacon_create(&buf, net_key_pairs[i].secondary, 0x00, 0x0000);
		send_beacon(&buf);
		ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					     beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1,
					    beacon_confirm_by_subnet, net_id_secondary));
		ASSERT_EQUAL(0x00, beacon.flags);
		ASSERT_EQUAL(0x0000, beacon.iv_index);

		/* Wait for end of sending all beacons from the rx node before sending beacon back
		 * to prevent beacon collision.
		 */
		k_sleep(K_MSEC(500));
	}

	/* Create a valid beacon secured with unknown Net Key. The node shall ignore the beacon and
	 * continue sending beacons regularly.
	 */
	uint8_t unknown_net_key[16] = {0xde, 0xad, 0xbe, 0xef};

	beacon_create(&buf, unknown_net_key, 0x00, 0x0000);
	send_beacon(&buf);
	/* Ensure that interval is not affected. */
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, beacon_confirm_all_subnets,
				    NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, beacon_confirm_all_subnets,
				    NULL));

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
	bt_mesh_crypto_init();
	k_sem_init(&observer_sem, 0, 1);
	k_work_init_delayable(&beacon_timer, secure_beacon_send);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
	}

	beacon_create(&buf, test_net_key, 0, 0);
	k_sleep(K_SECONDS(5));
	/*wait provisioned tx node to send the first beacon*/
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	k_sleep(K_SECONDS(2));

	/**
	 * Sending 2 SNB 20ms apart by only sending for even values of loop variable.
	 * And verify that tx node adapts to 20s SNB interval after sending enough
	 * beacons in for loop.
	 */
	for (size_t i = 1; i < 5; i++) {
		if (i % 2) {
			send_beacon(&buf);
			ASSERT_FALSE(
				wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
		} else {
			ASSERT_TRUE(
				wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
		}
	}

	/**
	 * Verify that tx node keeps the 20s SNB interval until adapts itself and
	 * sends SNB in 10s again.
	 */
	ASSERT_FALSE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
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
		if (wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL)) {
			delta = k_uptime_delta(&beacon_recv_time);
			break;
		}
	}

	ASSERT_TRUE(delta >= (600 * MSEC_PER_SEC));
	PASS();
}

static uint8_t snb_cnt;

static void snb_recv(const struct bt_mesh_snb *snb)
{
	/* IV idx of 2 marks end of test */
	if (snb->iv_idx == 2) {
		k_sem_give(&beacon_sem);
		return;
	}

	ASSERT_EQUAL(snb->flags, 0x02);
	ASSERT_EQUAL(snb->iv_idx, 1);
	snb_cnt++;
}

static void test_rx_beacon_cache(void)
{
	k_sem_init(&beacon_sem, 0, 1);
	snb_cb_ptr = snb_recv;

	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
	bt_mesh_test_setup();

	/* Wait for secondary SNB to end test. */
	ASSERT_OK_MSG(k_sem_take(&beacon_sem, K_SECONDS(40)),
		      "Didn't receive SNB in time");

	/* Verify that only one SNB for IV_idx=1 was handled. */
	ASSERT_EQUAL(snb_cnt, 1);
	PASS();
}

static void test_tx_beacon_cache(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_crypto_init();
	ASSERT_OK_MSG(bt_enable(NULL), "Bluetooth init failed");

	NET_BUF_SIMPLE_DEFINE(iv1, 22);
	NET_BUF_SIMPLE_DEFINE(iv2, 22);
	beacon_create(&iv1, test_net_key, 0x02, 0x0001);
	beacon_create(&iv2, test_net_key, 0x02, 0x0002);

	/* Send two copies of the same SNB. */
	for (size_t i = 0; i < 2; i++) {
		k_sleep(K_SECONDS(5));
		send_beacon(&iv1);
	}

	/* Send secondary SNB to mark end of test. */
	k_sleep(K_SECONDS(5));
	send_beacon(&iv2);

	PASS();
}

typedef void (*priv_beacon_cb)(const struct bt_mesh_prb *prb);

static priv_beacon_cb priv_beacon_cb_ptr;

static void priv_received(const struct bt_mesh_prb *prb)
{
	if (priv_beacon_cb_ptr) {
		priv_beacon_cb_ptr(prb);
	}
}

BT_MESH_BEACON_CB_DEFINE(priv_beacon) = {
	.priv_received = priv_received,
};

static bool private_beacon_check(const uint8_t *net_id, void *ctx)
{
	bool ret;
	bool same_random = *(bool *)ctx;

	if (memcmp(beacon.adv_addr.a.val, last_beacon_adv_addr.a.val, BT_ADDR_SIZE) == 0) {
		return false;
	}

	memcpy(&last_beacon_adv_addr.a.val, beacon.adv_addr.a.val, BT_ADDR_SIZE);

	if (same_random) {
		ret = memcmp(beacon.random, last_random, 13) == 0;
	} else {
		ret = memcmp(beacon.random, last_random, 13) != 0;
	}

	memcpy(&last_random, beacon.random, 13);

	return ret;
}

static void provision(const struct bt_mesh_test_cfg *dev_cfg)
{
	int err;

	err = bt_mesh_provision(net_key, 0, 0, 0, dev_cfg->addr, dev_cfg->dev_key);
	if (err) {
		FAIL("Provisioning failed (err %d)", err);
	}
}

static void tx_priv_setup(void)
{
	uint8_t status;
	struct bt_mesh_priv_beacon val;
	int err;

	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&tx_cfg);

	val.enabled = 1;
	val.rand_interval = random_interval;

	err = bt_mesh_cfg_cli_beacon_set(0, tx_cfg.addr, 0, &status);
	if (err || status != 0) {
		FAIL("Beacon set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_priv_beacon_cli_set(0, tx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}
}

static void test_tx_priv_on_iv_update(void)
{
	tx_priv_setup();

	tx_on_iv_update_test();
}

static void test_tx_priv_on_key_refresh(void)
{
	tx_priv_setup();

	tx_on_key_refresh_test();
}

static void test_tx_priv_adv(void)
{
	uint8_t status;
	struct bt_mesh_priv_beacon val;
	int err;

	bt_mesh_test_cfg_set(NULL, BEACON_INTERVAL_WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&tx_cfg);

	err = bt_mesh_cfg_cli_beacon_set(0, tx_cfg.addr, 0, &status);
	if (err || status != 0) {
		FAIL("Beacon set failed (err %d, status %u)", err, status);
	}

	val.enabled = 1;
	val.rand_interval = 1;

	err = bt_mesh_priv_beacon_cli_set(0, tx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}

	k_sleep(K_SECONDS(6 * BEACON_INTERVAL));

	val.rand_interval = 0;

	err = bt_mesh_priv_beacon_cli_set(0, tx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}

	k_sleep(K_SECONDS(6 * BEACON_INTERVAL));

	val.rand_interval = 0;

	err = bt_mesh_priv_beacon_cli_set(0, tx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}

	k_sleep(K_SECONDS(6 * BEACON_INTERVAL));

	val.rand_interval = 3;

	err = bt_mesh_priv_beacon_cli_set(0, tx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}

	PASS();
}

static void test_rx_priv_adv(void)
{
	bool same_random;
	int err, i;

	bt_mesh_test_cfg_set(&rx_cfg, BEACON_INTERVAL_WAIT_TIME);
	k_sem_init(&observer_sem, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
	}

	expected_beacon = BEACON_TYPE_PRIVATE;

	same_random = false;
	/* TX device is advertising with Random Interval = 1  for 6 intervals
	 * and with Random Interval = 0 for another 6
	 */
	for (i = 0; i < 12; i++) {
		wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
				&same_random);
	}

	/* TX device is advertising with Random Interval = 3 */
	for (i = 0; i < 2; i++) {
		same_random = true;

		for (int j = 0; j < 2; j++) {
			wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
					&same_random);
		}

		k_sleep(K_SECONDS(BEACON_INTERVAL));

		/* Beacon random should change here */
		same_random = true;
		wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
				&same_random);
	}

	PASS();
}

static void private_beacon_create(struct net_buf_simple *buf, const uint8_t *net_key, uint8_t flags,
			  uint32_t iv_index)
{
	uint8_t net_id[8];
	uint8_t auth[8];
	uint8_t data[5];
	uint8_t random_val[13];
	int err;

	err = bt_mesh_k3(net_key, net_id);
	if (err) {
		FAIL("Unable to generate Net ID");
	}

	err = bt_mesh_private_beacon_key(net_key, &priv_beacon_key);
	if (err) {
		FAIL("Unable to generate beacon key");
	}

	bt_rand(random_val, sizeof(random_val));
	bt_mesh_beacon_encrypt(&priv_beacon_key, flags, iv_index,
			       random_val, data, auth);

	net_buf_simple_reset(buf);
	net_buf_simple_add_u8(buf, BEACON_TYPE_PRIVATE);
	net_buf_simple_add_mem(buf, random_val, 13);
	net_buf_simple_add_mem(buf, data, 5);
	net_buf_simple_add_mem(buf, auth, 8);
}

static void test_tx_priv_invalid(void)
{
	uint8_t fields_offsets[4] = {1, 14, 15, 19};

	NET_BUF_SIMPLE_DEFINE(buf, 27);
	int err;

	bt_mesh_test_cfg_set(&tx_cfg, 130);
	bt_mesh_crypto_init();
	k_sem_init(&observer_sem, 0, 1);

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
	}

	LOG_INF("Bluetooth initialized");

	/* Let the rx node send the first beacon. */
	k_sleep(K_SECONDS(5));

	/* Create a valid beacon with IV Update Flag set to 1 and new IV Index. */
	private_beacon_create(&buf, net_key, 0x02, 0x0001);

	expected_beacon = BEACON_TYPE_PRIVATE;

	corrupted_beacon_test(fields_offsets, ARRAY_SIZE(fields_offsets), &buf);

	PASS();
}

static void test_rx_priv_invalid(void)
{
	uint8_t status;
	struct bt_mesh_priv_beacon val;
	int err;

	bt_mesh_test_cfg_set(NULL, 130);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&rx_cfg);
	bt_mesh_iv_update_test(true);

	val.enabled = 1;
	val.rand_interval = random_interval;

	err = bt_mesh_cfg_cli_beacon_set(0, rx_cfg.addr, 0, &status);
	if (err || status != 0) {
		FAIL("Beacon set failed (err %d, status %u)", err, status);
	}

	err = bt_mesh_priv_beacon_cli_set(0, rx_cfg.addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}

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

static void toggle_priv_beacon(uint16_t addr, uint8_t enabled)
{
	int err;
	uint8_t status;
	struct bt_mesh_priv_beacon val;

	err = bt_mesh_cfg_cli_beacon_set(0, addr, !enabled, &status);
	if (err || status != !enabled) {
		FAIL("Beacon set failed (err %d, status %u)", err, status);
	}

	val.enabled = enabled;
	val.rand_interval = 1;

	err = bt_mesh_priv_beacon_cli_set(0, addr, &val, &val);
	if (err) {
		FAIL("Failed to enable Private Beacon (err=%d)", err);
	}
}

static void test_tx_priv_interleave(void)
{
	uint8_t phase;
	uint8_t status;
	struct bt_mesh_subnet *sub;


	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&tx_cfg);

	sub = bt_mesh_subnet_get(0);
	ASSERT_TRUE(sub != NULL);

	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_INITIATOR));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_PENDING));
	ASSERT_TRUE(!atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));
	ASSERT_TRUE(bt_mesh.iv_index == 0);

	status = bt_mesh_subnet_kr_phase_get(0, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_NORMAL);

	/* Wait for SNB being advertised and switch beacon type between Beacon Intervals */
	k_sleep(K_SECONDS(BEACON_INTERVAL + 5));

	toggle_priv_beacon(tx_cfg.addr, 1);

	bt_mesh_iv_update_test(true);
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_TEST));

	ASSERT_TRUE(bt_mesh_iv_update());
	ASSERT_TRUE(atomic_test_bit(bt_mesh.flags, BT_MESH_IVU_IN_PROGRESS));
	ASSERT_TRUE(bt_mesh.iv_index == 1);

	/* Switch beacon type between Beacon Intervals */
	k_sleep(K_SECONDS(BEACON_INTERVAL + 5));

	toggle_priv_beacon(tx_cfg.addr, 0);
	/* Small delay to let beacons complete before subnet update is applied */
	k_sleep(K_MSEC(20));

	status = bt_mesh_subnet_update(BT_MESH_KEY_PRIMARY, net_key_new);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_1);

	phase = BT_MESH_KR_PHASE_2;
	status = bt_mesh_subnet_kr_phase_set(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	status = bt_mesh_subnet_kr_phase_get(BT_MESH_KEY_PRIMARY, &phase);
	ASSERT_TRUE(status == STATUS_SUCCESS);
	ASSERT_TRUE(phase == BT_MESH_KR_PHASE_2);

	k_sleep(K_SECONDS(BEACON_INTERVAL + 7));
	toggle_priv_beacon(tx_cfg.addr, 1);

	PASS();
}

static void test_rx_priv_interleave(void)
{
	int err;
	bool same_random = false;

	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
	bt_mesh_crypto_init();
	k_sem_init(&observer_sem, 0, 1);

	err = bt_mesh_private_beacon_key(net_key, &priv_beacon_key);
	if (err) {
		FAIL("Unable to generate beacon key");
	}

	err = bt_enable(NULL);
	if (err) {
		FAIL("Bluetooth init failed (err %d)", err);
	}

	expected_beacon = BEACON_TYPE_SECURE;
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));

	expected_beacon = BEACON_TYPE_PRIVATE;
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
				    &same_random));

	/* IVU was started here */
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
				    &same_random));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	memset(&beacon, 0, sizeof(beacon));
	expected_beacon = BEACON_TYPE_SECURE;

	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x02, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	/* KR was started here */
	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, NULL, NULL));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	expected_beacon = BEACON_TYPE_PRIVATE;

	err = bt_mesh_private_beacon_key(net_key_new, &priv_beacon_key);

	ASSERT_TRUE(wait_for_beacon(beacon_scan_cb, BEACON_INTERVAL + 1, private_beacon_check,
				    &same_random));
	ASSERT_EQUAL(0x03, beacon.flags);
	ASSERT_EQUAL(0x0001, beacon.iv_index);

	PASS();
}

static uint8_t prb_cnt;

static void priv_beacon_recv(const struct bt_mesh_prb *prb)
{
	/* IV idx of 2 marks end of test */
	if (prb->iv_idx == 2) {
		k_sem_give(&beacon_sem);
		return;
	}

	ASSERT_EQUAL(prb->flags, 0x02);
	ASSERT_EQUAL(prb->iv_idx, 1);
	prb_cnt++;
}

static void test_rx_priv_beacon_cache(void)
{
	k_sem_init(&beacon_sem, 0, 1);
	priv_beacon_cb_ptr = priv_beacon_recv;

	bt_mesh_test_cfg_set(&rx_cfg, WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&rx_cfg);

	/* Wait for secondary private beacon to end test. */
	ASSERT_OK_MSG(k_sem_take(&beacon_sem, K_SECONDS(40)),
		      "Didn't receive private beacon in time");

	/* Verify that only one private beacon for IV_idx=1 was handled. */
	ASSERT_EQUAL(prb_cnt, 1);
	PASS();
}

static void test_tx_priv_beacon_cache(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_crypto_init();
	ASSERT_OK_MSG(bt_enable(NULL), "Bluetooth init failed");

	NET_BUF_SIMPLE_DEFINE(iv1, 27);
	NET_BUF_SIMPLE_DEFINE(iv2, 27);
	private_beacon_create(&iv1, test_net_key, 0x02, 0x0001);
	private_beacon_create(&iv2, test_net_key, 0x02, 0x0002);

	/* Send two copies of the same private beacon. */
	for (size_t i = 0; i < 2; i++) {
		k_sleep(K_SECONDS(5));
		send_beacon(&iv1);
	}

	/* Send secondary private beacon to mark end of test. */
	k_sleep(K_SECONDS(5));
	send_beacon(&iv2);

	PASS();
}

#if defined(CONFIG_BT_MESH_GATT_PROXY)

static uint8_t test_net_key_3[16] = {0x12, 0x54, 0xab, 0x1e};

#define UNTIL_UPTIME(time) (k_uptime_get() > (time) ? K_NO_WAIT : K_MSEC((time) - k_uptime_get()))
#define BEACON_TYPE_NET_ID  0
#define BEACON_TYPE_NODE_ID 1
#define BEACON_TYPE_PRIVATE_NET_ID  2
#define BEACON_TYPE_PRIVATE_NODE_ID 3
#define BEACON_TYPE_PRIVATE_LEN     28
#define TEST_NET_IDX1               0
#define TEST_NET_IDX2               1
#define TEST_NET_IDX3               2
#define MAX_TIMEOUT ((CONFIG_BT_MESH_NODE_ID_TIMEOUT * 1000) / 6)

#define PP_NET_ID_WAIT_TIME 610 /*seconds*/
#define PP_NODE_ID_WAIT_TIME 80 /*seconds*/
#define PP_MULT_NET_ID_WAIT_TIME 50 /*seconds*/
#define PROXY_ADV_MULTI_SUBNET_COEX_WAIT_TIME 151 /*seconds*/

struct netkey_ctx {
	uint8_t *net_key;
	uint8_t net_id[8];
	uint8_t net_idx;
	struct bt_mesh_key id_key;
};

static struct netkey_ctx pp_net0 = {.net_key = (uint8_t *)test_net_key, .net_idx = 0};
static struct netkey_ctx pp_net1 = {.net_key = (uint8_t *)test_net_key_2, .net_idx = 1};
static struct netkey_ctx pp_net2 = {.net_key = (uint8_t *)test_net_key_3, .net_idx = 2};

struct priv_test_ctx {
	uint8_t beacon_type;
	uint16_t *node_id_addr;
};

static void pp_netkey_ctx_init(struct netkey_ctx *net)
{
	ASSERT_OK_MSG(bt_mesh_identity_key(net->net_key, &net->id_key),
		      "Failed to generate ID key");
	ASSERT_OK_MSG(bt_mesh_k3(net->net_key, net->net_id), "Failed to generate Net ID");
}

static uint8_t proxy_adv_type_get(uint8_t adv_type, struct net_buf_simple *buf)
{
	uint8_t type;
	uint8_t len = buf->len;

	if (adv_type != BT_GAP_ADV_TYPE_ADV_IND || len < 12) {
		return 0xFF;
	}

	(void)net_buf_simple_pull_mem(buf, 11);
	type = net_buf_simple_pull_u8(buf);
	/* BEACON_TYPE_NET_ID is 20 bytes long, while the three other accepted types are 28 bytes*/
	if (len != ((type == BEACON_TYPE_NET_ID) ? 20 : 28)) {
		return 0xFF;
	}

	return type;
}

static uint64_t proxy_adv_hash_calc(struct netkey_ctx *net, uint64_t random, uint16_t *addr,
				    bool is_priv)
{
	uint64_t hash;
	uint8_t tmp[16] = {0};

	tmp[5] = is_priv ? 3 : 0;

	if (addr) {
		memcpy(&tmp[6], &random, 8);
		sys_put_be16(*addr, &tmp[14]);

	} else {
		memcpy(&tmp[0], net->net_id, 8);
		memcpy(&tmp[8], &random, 8);
	}

	bt_mesh_encrypt(&net->id_key, tmp, tmp);
	memcpy(&hash, &tmp[8], 8);

	return hash;
}

static bool pp_beacon_check(const uint8_t *net_id, void *ctx)
{
	struct priv_test_ctx *test_ctx = (struct priv_test_ctx *)ctx;

	ASSERT_EQUAL(beacon.pp_hash,
		     proxy_adv_hash_calc(&pp_net0, beacon.pp_random, test_ctx->node_id_addr, true));

	if (memcmp(beacon.adv_addr.a.val, last_beacon_adv_addr.a.val, BT_ADDR_SIZE) == 0) {
		return false;
	}

	memcpy(&last_beacon_adv_addr.a.val, beacon.adv_addr.a.val, BT_ADDR_SIZE);

	return true;
}

static void priv_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
				struct net_buf_simple *buf)
{
	struct priv_test_ctx *ctx = (struct priv_test_ctx *)beacon.user_ctx;

	if (proxy_adv_type_get(adv_type, buf) != ctx->beacon_type) {
		/* Wrong message type */
		return;
	}

	bt_addr_le_copy(&beacon.adv_addr, addr);

	if (ctx->beacon_type == BEACON_TYPE_NET_ID) {
		beacon.net_id = net_buf_simple_pull_le64(buf);
	} else {
		beacon.pp_hash = net_buf_simple_pull_le64(buf);
		beacon.pp_random = net_buf_simple_pull_le64(buf);
	}

	if (!beacon.process_cb || beacon.process_cb(NULL, beacon.user_ctx)) {
		k_sem_give(&observer_sem);
	}
}

struct proxy_adv_beacon {
	uint8_t evt_type;
	uint8_t net_idx;
	int64_t rx_timestamp;
	union {
		uint64_t net_id;
		struct {
			uint64_t hash;
			uint64_t random;
		} enc;
	} ctx;
};

static void proxy_adv_scan_all_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
				struct net_buf_simple *buf)
{
	struct proxy_adv_beacon *beac = (struct proxy_adv_beacon *)beacon.user_ctx;

	beac->evt_type = proxy_adv_type_get(adv_type, buf);
	if (beac->evt_type == 0xFF) {
		/* Not a related beacon type */
		return;
	}

	bt_addr_le_copy(&beacon.adv_addr, addr);
	beac->rx_timestamp = k_uptime_get();

	if (beac->evt_type == BEACON_TYPE_NET_ID) {
		beac->ctx.net_id = net_buf_simple_pull_le64(buf);
	} else {
		beac->ctx.enc.hash = net_buf_simple_pull_le64(buf);
		beac->ctx.enc.random = net_buf_simple_pull_le64(buf);
	}

	if (!beacon.process_cb || beacon.process_cb(NULL, beacon.user_ctx)) {
		k_sem_give(&observer_sem);
	}
}

static void rx_priv_common_init(uint16_t wait)
{
	bt_mesh_test_cfg_set(&rx_cfg, wait);
	bt_mesh_crypto_init();
	pp_netkey_ctx_init(&pp_net0);
	k_sem_init(&observer_sem, 0, 1);
	ASSERT_OK_MSG(bt_enable(NULL), "Bluetooth init failed");
}

static void tx_proxy_adv_common_init(uint16_t wait, const struct bt_mesh_test_cfg *cfg)
{
	bt_mesh_test_cfg_set(NULL, wait);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(cfg);

	/* Disable GATT proxy */
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_DISABLED),
		      "Failed to disable gatt proxy");
}

static void test_tx_priv_net_id(void)
{
	tx_proxy_adv_common_init(PP_NET_ID_WAIT_TIME, &tx_cfg);

	/* Enable private GATT proxy */
	ASSERT_OK_MSG(bt_mesh_priv_gatt_proxy_set(BT_MESH_GATT_PROXY_ENABLED),
		      "Failed to set private gatt proxy");

	PASS();
}

static void test_rx_priv_net_id(void)
{
	struct priv_test_ctx ctx = {
		.beacon_type = BEACON_TYPE_PRIVATE_NET_ID,
		.node_id_addr = NULL,
	};

	rx_priv_common_init(PP_NET_ID_WAIT_TIME);

	/* Scan for first net ID */
	ASSERT_TRUE(
		wait_for_beacon(priv_scan_cb, 5, pp_beacon_check, &ctx));

	uint64_t last_pp_random = beacon.pp_random;

	/* Wait for 10 minutes, then scan for another net
	 * ID beacon and verify that random field has changed
	 */
	k_sleep(K_SECONDS(600));
	ASSERT_TRUE(
		wait_for_beacon(priv_scan_cb, 5, pp_beacon_check, &ctx));
	ASSERT_FALSE(beacon.pp_random == last_pp_random);

	PASS();
}

static void test_tx_priv_node_id(void)
{
	enum bt_mesh_feat_state state;

	tx_proxy_adv_common_init(PP_NODE_ID_WAIT_TIME, &tx_cfg);

	/* Start first node advertisement */
	ASSERT_OK_MSG(bt_mesh_subnet_priv_node_id_set(TEST_NET_IDX1, BT_MESH_NODE_IDENTITY_RUNNING),
		      "Failed to set private node ID");

	/* Wait for Node ID advertising to end */
	k_sleep(K_SECONDS(65));

	/* Check that advertisment has stopped */
	ASSERT_OK_MSG(bt_mesh_subnet_priv_node_id_get(TEST_NET_IDX1, &state),
		      "Failed to get private node ID");
	ASSERT_EQUAL(state, BT_MESH_NODE_IDENTITY_STOPPED);

	/* Start second node advertisement */
	ASSERT_OK_MSG(bt_mesh_subnet_priv_node_id_set(TEST_NET_IDX1, BT_MESH_NODE_IDENTITY_RUNNING),
		      "Failed to set private node ID");

	/* Wait to let node ID advertise for a while */
	k_sleep(K_SECONDS(5));

	PASS();
}

static void test_rx_priv_node_id(void)
{
	struct priv_test_ctx ctx = {
		.beacon_type = BEACON_TYPE_PRIVATE_NODE_ID,
		.node_id_addr = (uint16_t *)&tx_cfg.addr,
	};

	rx_priv_common_init(PP_NODE_ID_WAIT_TIME);

	/* Scan for first node ID */
	ASSERT_TRUE(
		wait_for_beacon(priv_scan_cb, 5, pp_beacon_check, &ctx));

	uint64_t last_pp_random = beacon.pp_random;

	/* Wait for first node ID advertisment to finish, then scan for
	 * second node ID and verify that random field has changed
	 */

	k_sleep(K_SECONDS(65));
	ASSERT_TRUE(
		wait_for_beacon(priv_scan_cb, 5, pp_beacon_check, &ctx));
	ASSERT_FALSE(beacon.pp_random == last_pp_random);

	PASS();
}

static void test_tx_priv_multi_net_id(void)
{
	tx_proxy_adv_common_init(PP_MULT_NET_ID_WAIT_TIME, &tx_cfg);

	/* Add second network */
	ASSERT_OK_MSG(bt_mesh_subnet_add(TEST_NET_IDX2, test_net_key_2),
		      "Failed to add second subnet");

	/* Enable private GATT proxy */
	ASSERT_OK_MSG(bt_mesh_priv_gatt_proxy_set(BT_MESH_GATT_PROXY_ENABLED),
		      "Failed to set private gatt proxy");

	PASS();
}

static void proxy_adv_subnet_find(struct proxy_adv_beacon *beac, struct netkey_ctx **nets,
				  uint8_t net_cnt)
{
	for (size_t i = 0; i < net_cnt; i++) {

		switch (beac->evt_type) {
		case BEACON_TYPE_NET_ID:
			if (!memcmp(nets[i]->net_id, &beac->ctx.net_id, 8)) {
				beac->net_idx = nets[i]->net_idx;
				return;
			}
			break;
		case BEACON_TYPE_NODE_ID:
			if (beac->ctx.enc.hash ==
			    proxy_adv_hash_calc(nets[i], beac->ctx.enc.random,
						(uint16_t *)&tx_cfg.addr, false)) {
				beac->net_idx = nets[i]->net_idx;
				return;
			}
			break;
		case BEACON_TYPE_PRIVATE_NET_ID:
			if (beac->ctx.enc.hash ==
			    proxy_adv_hash_calc(nets[i], beac->ctx.enc.random,
						NULL, true)) {
				beac->net_idx = nets[i]->net_idx;
				return;
			}
			break;
		case BEACON_TYPE_PRIVATE_NODE_ID:
			if (beac->ctx.enc.hash ==
			    proxy_adv_hash_calc(nets[i], beac->ctx.enc.random,
						(uint16_t *)&tx_cfg.addr, true)) {
				beac->net_idx = nets[i]->net_idx;
				return;
			}
			break;

		default:
			FAIL("Unexpected beacon type");
			break;
		}
	}

	FAIL("Could not find matching subnet for incoming proxy adv beacon");
}

static const char *const proxy_adv_str[] = {"Net_ID", "Node_ID", "Priv_Net_ID", "Priv_Node_ID"};
struct expected_proxy_adv_evt {
	uint8_t evt_type;
	uint8_t net_idx;
	uint16_t evt_cnt;
	struct {
		int64_t after;
		int64_t before;
	} time;
};

static void proxy_adv_register_evt(struct proxy_adv_beacon *beac,
				struct expected_proxy_adv_evt *exp_evts, uint8_t cnt)
{
	for (int i = 0; i < cnt; i++) {
		if ((exp_evts[i].evt_cnt) && (beac->evt_type == exp_evts[i].evt_type) &&
		    (beac->net_idx == exp_evts[i].net_idx) &&
		    (beac->rx_timestamp >= exp_evts[i].time.after) &&
		    (beac->rx_timestamp <= exp_evts[i].time.before)) {
			exp_evts[i].evt_cnt--;
		}
	}
}

static void proxy_adv_confirm_evt(struct expected_proxy_adv_evt *exp_evts, uint8_t cnt)
{
	bool missing_evts = false;

	for (int i = 0; i < cnt; i++) {
		if (exp_evts[i].evt_cnt) {
			LOG_ERR("Missing %d expected %s idx %d events in period %llums-%llums",
				exp_evts[i].evt_cnt, proxy_adv_str[exp_evts[i].evt_type],
				exp_evts[i].net_idx, exp_evts[i].time.after,
				exp_evts[i].time.before);
			missing_evts = true;
		}
	}

	if (missing_evts) {
		FAIL("Test failed due to missing events");
	}
}

static void proxy_adv_scan_all(struct netkey_ctx **nets, uint16_t net_cnt,
			       struct expected_proxy_adv_evt *exp_evt, uint16_t exp_evt_cnt,
			       int64_t timeout)
{
	struct proxy_adv_beacon beac;

	while (k_uptime_get() < timeout) {

		ASSERT_TRUE(wait_for_beacon(proxy_adv_scan_all_cb, 2, NULL, &beac));
		proxy_adv_subnet_find(&beac, nets, net_cnt);
		proxy_adv_register_evt(&beac, exp_evt, exp_evt_cnt);

		/** We want to monitor an even distribution of adv events.
		 *  To ensure this, we wait a little less than the minimum
		 *  proxy adv period (1 second) before scanning for the next
		 *  evt.
		 */
		k_sleep(K_MSEC(990));
	}

	proxy_adv_confirm_evt(exp_evt, exp_evt_cnt);
}

#define PROXY_ADV_MULTI_CHECKPOINT_1 20000
#define PROXY_ADV_MULTI_CHECKPOINT_2 50000
#define PROXY_ADV_MULTI_CHECKPOINT_3 110000
#define PROXY_ADV_MULTI_CHECKPOINT_4 130000
#define PROXY_ADV_MULTI_CHECKPOINT_END 150000

static void test_tx_proxy_adv_multi_subnet_coex(void)
{
	tx_proxy_adv_common_init(PROXY_ADV_MULTI_SUBNET_COEX_WAIT_TIME, &tx_cfg);

	/* Enable GATT proxy */
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_ENABLED),
		      "Failed to Enable gatt proxy");

	k_sleep(UNTIL_UPTIME(PROXY_ADV_MULTI_CHECKPOINT_1));
	/* Add second and third network */
	ASSERT_OK_MSG(bt_mesh_subnet_add(TEST_NET_IDX2, test_net_key_2),
		      "Failed to add second subnet");
	ASSERT_OK_MSG(bt_mesh_subnet_add(TEST_NET_IDX3, test_net_key_3),
		      "Failed to add third subnet");

	k_sleep(UNTIL_UPTIME(PROXY_ADV_MULTI_CHECKPOINT_2));
	/* Start Node Identity on second network */
	bt_mesh_proxy_identity_start(bt_mesh_subnet_get(TEST_NET_IDX2), false);

	k_sleep(UNTIL_UPTIME(PROXY_ADV_MULTI_CHECKPOINT_3));
	/* Prepare for solicitation */
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_DISABLED),
		      "Failed to Enable gatt proxy");
	ASSERT_OK_MSG(bt_mesh_od_priv_proxy_set(20), "Failed to set OD priv proxy state");

	k_sleep(UNTIL_UPTIME(PROXY_ADV_MULTI_CHECKPOINT_4));
	/* Re-enable GATT proxy and remove second and third network */
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_ENABLED),
		      "Failed to Enable gatt proxy");
	ASSERT_OK_MSG(bt_mesh_subnet_del(TEST_NET_IDX2), "Failed to delete subnet");
	ASSERT_OK_MSG(bt_mesh_subnet_del(TEST_NET_IDX3), "Failed to delete subnet");

	PASS();
}

static const struct bt_mesh_test_cfg solicit_trigger_cfg = {
	.addr = 0x0003,
	.dev_key = { 0x03 },
};

static void test_tx_proxy_adv_solicit_trigger(void)
{
	tx_proxy_adv_common_init(PROXY_ADV_MULTI_SUBNET_COEX_WAIT_TIME, &solicit_trigger_cfg);
	/* Disable SNB. */
	bt_mesh_beacon_set(false);
	ASSERT_OK_MSG(bt_mesh_subnet_add(TEST_NET_IDX2, test_net_key_2),
		      "Failed to add second subnet");

	k_sleep(UNTIL_UPTIME(PROXY_ADV_MULTI_CHECKPOINT_3));

	/* Solicit first and second network */
	ASSERT_OK_MSG(bt_mesh_proxy_solicit(TEST_NET_IDX1),
		      "Failed to start solicitation");
	ASSERT_OK_MSG(bt_mesh_proxy_solicit(TEST_NET_IDX2),
		      "Failed to start solicitation");

	PASS();
}

static void test_rx_proxy_adv_multi_subnet_coex(void)
{
	rx_priv_common_init(PROXY_ADV_MULTI_SUBNET_COEX_WAIT_TIME);
	/* Disable SNB. */
	bt_mesh_beacon_set(false);
	pp_netkey_ctx_init(&pp_net1);
	pp_netkey_ctx_init(&pp_net2);

	struct netkey_ctx *nets[] = {&pp_net0, &pp_net1, &pp_net2};
	struct expected_proxy_adv_evt exp_evt[] = {
		/** A single subnet is active on the device with GATT Proxy
		 *  enabled. Verify that the single subnet has exclusive
		 *  access to the adv medium.
		 */
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 0, .evt_cnt = 19,
		 .time = {.after = 0, .before = PROXY_ADV_MULTI_CHECKPOINT_1}},

		/** Two additional subnets are added to the device.
		 *  Check that the subnets are sharing the adv medium,
		 *  advertising NET_ID beacons.
		 */
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 0, .evt_cnt = 8,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_1,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_2}},
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 1, .evt_cnt = 8,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_1,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_2}},
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 2, .evt_cnt = 8,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_1,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_2}},

		/** The second subnet enables Node Identity. Check that NODE_ID
		 *  is advertised by this subnet, and that the two others
		 *  continues to advertise NET_ID.
		 */
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 0, .evt_cnt = 16,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_2,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_3}},
		{.evt_type = BEACON_TYPE_NODE_ID, .net_idx = 1, .evt_cnt = 16,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_2,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_3}},
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 2, .evt_cnt = 16,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_2,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_3}},

		/** The first and second subnet gets solicited. Check that
		 *  PRIVATE_NET_ID is advertised by these subnet,
		 */
		{.evt_type = BEACON_TYPE_PRIVATE_NET_ID, .net_idx = 0, .evt_cnt = 8,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_3,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_4}},
		{.evt_type = BEACON_TYPE_PRIVATE_NET_ID, .net_idx = 1, .evt_cnt = 8,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_3,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_4}},

		/** Second and third subnet are disabled. Verify that the single
		 *  subnet has exclusive access to the adv medium.
		 */
		{.evt_type = BEACON_TYPE_NET_ID, .net_idx = 0, .evt_cnt = 18,
		 .time = {.after = PROXY_ADV_MULTI_CHECKPOINT_4,
			  .before = PROXY_ADV_MULTI_CHECKPOINT_END}},
	};

	proxy_adv_scan_all(nets, ARRAY_SIZE(nets), exp_evt, ARRAY_SIZE(exp_evt),
			   PROXY_ADV_MULTI_CHECKPOINT_END);
	PASS();
}

static void test_rx_priv_multi_net_id(void)
{
	rx_priv_common_init(PP_MULT_NET_ID_WAIT_TIME);
	pp_netkey_ctx_init(&pp_net1);

	struct priv_test_ctx ctx = {
		.beacon_type = BEACON_TYPE_PRIVATE_NET_ID,
		.node_id_addr = NULL,
	};

	uint16_t itr = 4;
	static uint8_t old_idx = 0xff;
	static struct {
		struct netkey_ctx *net;
		uint16_t recv_cnt;
		int64_t start;
	} net_ctx[2] = {
		{.net = &pp_net0},

		{.net = &pp_net1},
	};

	while (itr) {
		/* Scan for net ID from both networks  */
		ASSERT_TRUE(wait_for_beacon(priv_scan_cb, 5, NULL, &ctx));

		for (size_t i = 0; i < ARRAY_SIZE(net_ctx); i++) {
			if (beacon.pp_hash ==
			    proxy_adv_hash_calc(net_ctx[i].net, beacon.pp_random, NULL, true)) {
				if (old_idx == 0xff) {
					/* Received first Net ID advertisment */
					old_idx = i;
					net_ctx[i].start = k_uptime_get();
					net_ctx[i].recv_cnt++;
				} else if (old_idx != i) {
					/* Received Net ID adv for new subnet */

					/* Verify last Net ID adv result */
					ASSERT_IN_RANGE(k_uptime_get() - net_ctx[old_idx].start,
							MAX_TIMEOUT - 1000, MAX_TIMEOUT + 1000);
					ASSERT_IN_RANGE(net_ctx[old_idx].recv_cnt, 9, 12);
					net_ctx[old_idx].recv_cnt = 0;
					old_idx = i;

					/* The test ends when all itterations are completed */
					itr--;

					net_ctx[i].start = k_uptime_get();
					net_ctx[i].recv_cnt++;
				} else {
					/* Received another Net ID adv from same subnet*/
					net_ctx[i].recv_cnt++;
				}

				break;
			}
		}
	}

	PASS();
}

static void test_tx_priv_gatt_proxy(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&tx_cfg);
	bt_mesh_iv_update_test(true);

	ASSERT_TRUE(bt_mesh.iv_index == 0);

	/* Disable SNB. */
	bt_mesh_beacon_set(false);
	ASSERT_OK_MSG(bt_mesh_scan_disable(), "Failed to disable scanner");
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_DISABLED),
		      "Failed to disable gatt proxy");
	ASSERT_OK_MSG(bt_mesh_priv_gatt_proxy_set(BT_MESH_PRIV_GATT_PROXY_ENABLED),
		      "Failed to set private gatt proxy");

	/* Wait for proxy connection to complete. */
	WAIT_FOR_COND(bt_mesh_proxy_srv_connected_cnt() == 1, 10);

	/* Wait a bit so RX device can disable scanner, then start IV update */
	k_sleep(K_SECONDS(2));
	ASSERT_TRUE(bt_mesh_iv_update());

	/* Check that IV index has updated */
	ASSERT_TRUE(bt_mesh.iv_index == 1);
	PASS();
}

static void test_rx_priv_gatt_proxy(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
	bt_mesh_device_setup(&prov, &prb_comp);
	provision(&rx_cfg);
	bt_mesh_iv_update_test(true);

	ASSERT_TRUE(bt_mesh.iv_index == 0);

	/* Disable SNB. */
	bt_mesh_beacon_set(false);
	ASSERT_OK_MSG(bt_mesh_gatt_proxy_set(BT_MESH_GATT_PROXY_DISABLED),
		      "Failed to disable gatt proxy");
	ASSERT_OK_MSG(bt_mesh_priv_gatt_proxy_set(BT_MESH_PRIV_GATT_PROXY_ENABLED),
		      "Failed to set private gatt proxy");
	ASSERT_OK_MSG(bt_mesh_proxy_connect(TEST_NET_IDX1), "Failed to connect over proxy");

	/* Wait for connection to complete, then disable scanner
	 * to ensure that all RX communication arrives over GATT.
	 */
	WAIT_FOR_COND(bt_mesh_proxy_cli_is_connected(TEST_NET_IDX1), 10);
	ASSERT_OK_MSG(bt_mesh_scan_disable(), "Failed to disable scanner");

	/* Wait for the IV index to update.
	 * Verifying that IV index has changed proves that a private
	 * beacon arrived successfully over the GATT connection.
	 */
	WAIT_FOR_COND(bt_mesh.iv_index == 1, 10);

	PASS();
}

#endif

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "beacon_" #role "_" #name,          \
		.test_descr = description,                     \
		.test_pre_init_f = test_##role##_init,         \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
		.test_args_f = test_args_parse,                \
	}

static const struct bst_test_instance test_beacon[] = {
	TEST_CASE(tx, on_iv_update,   "Beacon: send on IV update"),
	TEST_CASE(tx, on_key_refresh,  "Beacon: send on key refresh"),
	TEST_CASE(tx, invalid, "Beacon: send invalid beacon"),
	TEST_CASE(tx, kr_old_key, "Beacon: send old Net Key"),
	TEST_CASE(tx, multiple_netkeys, "Beacon: multiple Net Keys"),
	TEST_CASE(tx, secure_beacon_interval, "Beacon: send secure beacons"),
	TEST_CASE(tx, beacon_cache,   "Beacon: advertise duplicate SNBs"),
	TEST_CASE(tx, priv_on_iv_update,   "Private Beacon: send on IV update"),
	TEST_CASE(tx, priv_on_key_refresh,   "Private Beacon: send on Key Refresh"),
	TEST_CASE(tx, priv_adv,   "Private Beacon: advertise Private Beacons"),
	TEST_CASE(tx, priv_invalid,   "Private Beacon: advertise invalid beacons"),
	TEST_CASE(tx, priv_interleave,   "Private Beacon: advertise interleaved with SNB"),
	TEST_CASE(tx, priv_beacon_cache,   "Private Beacon: advertise duplicate Private Beacons"),
#if CONFIG_BT_MESH_GATT_PROXY
	TEST_CASE(tx, priv_net_id,   "Private Proxy: advertise Net ID"),
	TEST_CASE(tx, priv_node_id,   "Private Proxy: advertise Node ID"),
	TEST_CASE(tx, priv_multi_net_id,   "Private Proxy: advertise multiple Net ID"),
	TEST_CASE(tx, priv_gatt_proxy,   "Private Proxy: Send Private Beacons over GATT"),
	TEST_CASE(tx, proxy_adv_multi_subnet_coex,   "Proxy Adv: Multi subnet coex proxy adv"),
	TEST_CASE(tx, proxy_adv_solicit_trigger,   "Proxy Adv: Trigger Solicitation"),
#endif

	TEST_CASE(rx, on_iv_update,   "Beacon: receive with IV update flag"),
	TEST_CASE(rx, on_key_refresh,  "Beacon: receive with key refresh flag"),
	TEST_CASE(rx, invalid, "Beacon: receive invalid beacon"),
	TEST_CASE(rx, kr_old_key, "Beacon: receive old Net Key"),
	TEST_CASE(rx, multiple_netkeys, "Beacon: multiple Net Keys"),
	TEST_CASE(rx, secure_beacon_interval, "Beacon: receive and send secure beacons"),
	TEST_CASE(rx, beacon_cache,   "Beacon: receive duplicate SNBs"),
	TEST_CASE(rx, priv_adv,   "Private Beacon: verify random regeneration"),
	TEST_CASE(rx, priv_invalid,   "Private Beacon: receive invalid beacons"),
	TEST_CASE(rx, priv_interleave,   "Private Beacon: interleaved with SNB"),
	TEST_CASE(rx, priv_beacon_cache,   "Private Beacon: receive duplicate Private Beacons"),
#if CONFIG_BT_MESH_GATT_PROXY
	TEST_CASE(rx, priv_net_id,   "Private Proxy: scan for Net ID"),
	TEST_CASE(rx, priv_node_id,   "Private Proxy: scan for Node ID"),
	TEST_CASE(rx, priv_multi_net_id,   "Private Proxy: scan for multiple Net ID"),
	TEST_CASE(rx, priv_gatt_proxy,   "Private Proxy: Receive Private Beacons over GATT"),
	TEST_CASE(rx, proxy_adv_multi_subnet_coex,   "Proxy Adv: Multi subnet coex proxy adv"),
#endif
	BSTEST_END_MARKER
};

struct bst_test_list *test_beacon_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_beacon);
	return tests;
}
