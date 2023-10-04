/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include "mesh_test.h"
#include "mesh/adv.h"
#include "mesh/net.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"

#define LOG_MODULE_NAME test_adv

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/

enum bt_mesh_gatt_service {
	MESH_SERVICE_PROVISIONING,
	MESH_SERVICE_PROXY,
};

struct bt_mesh_test_adv {
	uint8_t retr;     /* number of retransmits of adv frame */
	int64_t interval; /* interval of transmitted frames */
};

struct bt_mesh_test_gatt {
	uint8_t transmits; /* number of frame (pb gatt or proxy beacon) transmits */
	int64_t interval;  /* interval of transmitted frames */
	enum bt_mesh_gatt_service service;
};

extern const struct bt_mesh_comp comp;

static uint8_t test_prov_uuid[16] = { 0x6c, 0x69, 0x6e, 0x67, 0x61, 0xaa };

static const struct bt_mesh_test_cfg adv_cfg = {
	.addr = 0x0001,
	.dev_key = { 0x01 },
};

static struct bt_mesh_send_cb send_cb;
static struct bt_mesh_test_adv xmit_param;
static const char txt_msg[] = "adv test";
static const char cb_msg[] = "cb test";
static int64_t tx_timestamp;
static int seq_checker;
static struct bt_mesh_test_gatt gatt_param;
static int num_adv_sent;
static uint8_t previous_checker = 0xff;

static K_SEM_DEFINE(observer_sem, 0, 1);

static void test_tx_init(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
}

static void test_rx_init(void)
{
	bt_mesh_test_cfg_set(NULL, WAIT_TIME);
}

static void bt_init(void)
{
	ASSERT_OK_MSG(bt_enable(NULL), "Bluetooth init failed");
	LOG_INF("Bluetooth initialized");
}

static void adv_init(void)
{
	bt_mesh_adv_init();
	ASSERT_OK_MSG(bt_mesh_adv_enable(), "Mesh adv init failed");
}

static void allocate_all_array(struct net_buf **buf, size_t num_buf, uint8_t xmit)
{
	for (int i = 0; i < num_buf; i++) {
		*buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
					  xmit, K_NO_WAIT);

		ASSERT_FALSE(!*buf, "Out of buffers");
		buf++;
	}
}

static void verify_adv_queue_overflow(void)
{
	struct net_buf *dummy_buf;

	/* Verity Queue overflow */
	dummy_buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
				       BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_TRUE(!dummy_buf, "Unexpected extra buffer");
}

static bool check_delta_time(uint8_t transmit, uint64_t interval)
{
	static int cnt;
	static int64_t timestamp;

	if (cnt) {
		int64_t delta = k_uptime_delta(&timestamp);

		LOG_INF("rx: cnt(%d) delta(%dms)", cnt, delta);

		ASSERT_TRUE(delta >= interval &&
			    delta < (interval + 15));
	} else {
		timestamp = k_uptime_get();

		LOG_INF("rx: cnt(%d) delta(0ms)", cnt);
	}

	cnt++;

	if (cnt >= transmit) {
		cnt = 0;
		timestamp = 0;
		return true;
	}

	return false;
}

static void single_start_cb(uint16_t duration, int err, void *cb_data)
{
	int64_t delta;

	delta = k_uptime_delta(&tx_timestamp);
	LOG_INF("tx start: +%d ms", delta);
	ASSERT_TRUE(duration >= 90 && duration <= 200);
	ASSERT_EQUAL(0, err);
	ASSERT_EQUAL(cb_msg, cb_data);
	ASSERT_EQUAL(0, seq_checker & 1);
	seq_checker++;
}

static void single_end_cb(int err, void *cb_data)
{
	int64_t delta;

	delta = k_uptime_delta(&tx_timestamp);
	LOG_INF("tx end: +%d ms", delta);
	ASSERT_EQUAL(0, err);
	ASSERT_EQUAL(cb_msg, cb_data);
	ASSERT_EQUAL(1, seq_checker & 1);
	seq_checker++;
	k_sem_give(&observer_sem);
}

static void realloc_end_cb(int err, void *cb_data)
{
	struct net_buf *buf = (struct net_buf *)cb_data;

	ASSERT_EQUAL(0, err);
	buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
			BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_FALSE(!buf, "Out of buffers");

	k_sem_give(&observer_sem);
}

static void seq_start_cb(uint16_t duration, int err, void *cb_data)
{
	ASSERT_EQUAL(0, err);
	ASSERT_EQUAL(seq_checker, (intptr_t)cb_data);
}

static void seq_end_cb(int err, void *cb_data)
{
	ASSERT_EQUAL(0, err);
	ASSERT_EQUAL(seq_checker, (intptr_t)cb_data);
	seq_checker++;

	if (seq_checker == CONFIG_BT_MESH_ADV_BUF_COUNT) {
		k_sem_give(&observer_sem);
	}
}

static void parse_mesh_gatt_preamble(struct net_buf_simple *buf)
{
	ASSERT_EQUAL(0x0201, net_buf_simple_pull_be16(buf));
	/* flags */
	(void)net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(0x0303, net_buf_simple_pull_be16(buf));
}

static void parse_mesh_pb_gatt_service(struct net_buf_simple *buf)
{
	/* Figure 7.1: PB-GATT Advertising Data */
	/* mesh provisioning service */
	ASSERT_EQUAL(0x2718, net_buf_simple_pull_be16(buf));
	ASSERT_EQUAL(0x1516, net_buf_simple_pull_be16(buf));
	/* mesh provisioning service */
	ASSERT_EQUAL(0x2718, net_buf_simple_pull_be16(buf));
}

static void parse_mesh_proxy_service(struct net_buf_simple *buf)
{
	/* Figure 7.2: Advertising with Network ID (Identification Type 0x00) */
	/* mesh proxy service */
	ASSERT_EQUAL(0x2818, net_buf_simple_pull_be16(buf));
	ASSERT_EQUAL(0x0c16, net_buf_simple_pull_be16(buf));
	/* mesh proxy service */
	ASSERT_EQUAL(0x2818, net_buf_simple_pull_be16(buf));
	/* network ID */
	ASSERT_EQUAL(0x00, net_buf_simple_pull_u8(buf));
}

static void gatt_scan_cb(const bt_addr_le_t *addr, int8_t rssi,
			  uint8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	parse_mesh_gatt_preamble(buf);

	if (gatt_param.service == MESH_SERVICE_PROVISIONING) {
		parse_mesh_pb_gatt_service(buf);
	} else {
		parse_mesh_proxy_service(buf);
	}

	LOG_INF("rx: %s", txt_msg);

	if (check_delta_time(gatt_param.transmits, gatt_param.interval)) {
		LOG_INF("rx completed. stop observer.");
		k_sem_give(&observer_sem);
	}
}

static void rx_gatt_beacons(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_MESH_ADV_SCAN_UNIT(1000),
		.window     = BT_MESH_ADV_SCAN_UNIT(1000)
	};
	int err;

	err = bt_le_scan_start(&scan_param, gatt_scan_cb);
	ASSERT_FALSE(err && err != -EALREADY, "starting scan failed (err %d)", err);

	err = k_sem_take(&observer_sem, K_SECONDS(20));
	ASSERT_OK(err);

	err = bt_le_scan_stop();
	ASSERT_FALSE(err && err != -EALREADY, "stopping scan failed (err %d)", err);
}

static void xmit_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			struct net_buf_simple *buf)
{
	uint8_t length;

	if (adv_type != BT_GAP_ADV_TYPE_ADV_NONCONN_IND) {
		return;
	}

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(length, sizeof(uint8_t) + sizeof(txt_msg));
	ASSERT_EQUAL(BT_DATA_MESH_MESSAGE, net_buf_simple_pull_u8(buf));

	char *data = net_buf_simple_pull_mem(buf, sizeof(txt_msg));

	LOG_INF("rx: %s", txt_msg);
	ASSERT_EQUAL(0, memcmp(txt_msg, data, sizeof(txt_msg)));

	/* Add 1 initial transmit to the retransmit. */
	if (check_delta_time(xmit_param.retr + 1, xmit_param.interval)) {
		LOG_INF("rx completed. stop observer.");
		k_sem_give(&observer_sem);
	}
}

static void rx_xmit_adv(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_MESH_ADV_SCAN_UNIT(1000),
		.window     = BT_MESH_ADV_SCAN_UNIT(1000)
	};
	int err;

	err = bt_le_scan_start(&scan_param, xmit_scan_cb);
	ASSERT_FALSE(err && err != -EALREADY, "starting scan failed (err %d)", err);

	err = k_sem_take(&observer_sem, K_SECONDS(20));
	ASSERT_OK(err);

	err = bt_le_scan_stop();
	ASSERT_FALSE(err && err != -EALREADY, "stopping scan failed (err %d)", err);
}

static void send_order_start_cb(uint16_t duration, int err, void *user_data)
{
	struct net_buf *buf = (struct net_buf *)user_data;

	ASSERT_OK_MSG(err, "Failed adv start cb err (%d)", err);
	ASSERT_EQUAL(2, buf->len);

	uint8_t current = buf->data[0];
	uint8_t previous = buf->data[1];

	LOG_INF("tx start: current(%d) previous(%d)", current, previous);

	ASSERT_EQUAL(previous_checker, previous);
	previous_checker = current;
}

static void send_order_end_cb(int err, void *user_data)
{
	struct net_buf *buf = (struct net_buf *)user_data;

	ASSERT_OK_MSG(err, "Failed adv start cb err (%d)", err);
	ASSERT_TRUE(!buf->data, "Data not cleared!");
	seq_checker++;
	LOG_INF("tx end: seq(%d)", seq_checker);

	if (seq_checker == num_adv_sent) {
		seq_checker = 0;
		previous_checker = 0xff;
		k_sem_give(&observer_sem);
	}
}

static void receive_order_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
			struct net_buf_simple *buf)
{
	uint8_t length;
	uint8_t current;
	uint8_t previous;

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(BT_DATA_MESH_MESSAGE, net_buf_simple_pull_u8(buf));
	current = net_buf_simple_pull_u8(buf);
	previous = net_buf_simple_pull_u8(buf);
	LOG_INF("rx: current(%d) previous(%d)", current, previous);
	ASSERT_EQUAL(previous_checker, previous);

	/* Add 1 initial transmit to the retransmit. */
	if (check_delta_time(xmit_param.retr + 1, xmit_param.interval)) {
		previous_checker = current;
		k_sem_give(&observer_sem);
	}
}

static void receive_order(int expect_adv)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_PASSIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = BT_MESH_ADV_SCAN_UNIT(1000),
		.window     = BT_MESH_ADV_SCAN_UNIT(1000)
	};
	int err;

	err = bt_le_scan_start(&scan_param, receive_order_scan_cb);
	ASSERT_FALSE(err && err != -EALREADY, "starting scan failed (err %d)", err);

	previous_checker = 0xff;
	for (int i = 0; i < expect_adv; i++) {
		err = k_sem_take(&observer_sem, K_SECONDS(10));
		ASSERT_OK_MSG(err, "Didn't receive adv in time");
	}

	err = bt_le_scan_stop();
	ASSERT_FALSE(err && err != -EALREADY, "stopping scan failed (err %d)", err);
}

static void send_adv_buf(struct net_buf *buf, uint8_t curr, uint8_t prev)
{
	send_cb.start = send_order_start_cb;
	send_cb.end = send_order_end_cb;

	(void)net_buf_add_u8(buf, curr);
	(void)net_buf_add_u8(buf, prev);

	bt_mesh_adv_send(buf, &send_cb, buf);
	net_buf_unref(buf);
}

static void send_adv_array(struct net_buf **buf, size_t num_buf, bool reverse)
{
	uint8_t previous;
	int i;

	num_adv_sent = num_buf;
	previous = 0xff;
	if (!reverse) {
		i = 0;
	} else {
		i = num_buf - 1;
	}
	while ((!reverse && i < num_buf) || (reverse && i >= 0)) {
		send_adv_buf(*buf, (uint8_t)i, previous);
		previous = (uint8_t)i;
		if (!reverse) {
			buf++;
			i++;
		} else {
			buf--;
			i--;
		}
	}
}

static void test_tx_cb_single(void)
{
	struct net_buf *buf;
	int err;

	bt_init();
	adv_init();

	buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
			BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_FALSE(!buf, "Out of buffers");

	send_cb.start = single_start_cb;
	send_cb.end = single_end_cb;

	net_buf_add_mem(buf, txt_msg, sizeof(txt_msg));
	seq_checker = 0;
	tx_timestamp = k_uptime_get();
	bt_mesh_adv_send(buf, &send_cb, (void *)cb_msg);
	net_buf_unref(buf);

	err = k_sem_take(&observer_sem, K_SECONDS(1));
	ASSERT_OK_MSG(err, "Didn't call end tx cb.");

	PASS();
}

static void test_rx_xmit(void)
{
	xmit_param.retr = 2;
	xmit_param.interval = 20;

	bt_init();
	rx_xmit_adv();

	PASS();
}

static void test_tx_cb_multi(void)
{
	struct net_buf *buf[CONFIG_BT_MESH_ADV_BUF_COUNT];
	int err;

	bt_init();
	adv_init();

	/* Allocate all network buffers. */
	allocate_all_array(buf, ARRAY_SIZE(buf), BT_MESH_TRANSMIT(2, 20));

	/* Start single adv to reallocate one network buffer in callback.
	 * Check that the buffer is freed before cb is triggered.
	 */
	send_cb.start = NULL;
	send_cb.end = realloc_end_cb;
	net_buf_add_mem(buf[0], txt_msg, sizeof(txt_msg));

	bt_mesh_adv_send(buf[0], &send_cb, buf[0]);
	net_buf_unref(buf[0]);

	err = k_sem_take(&observer_sem, K_SECONDS(1));
	ASSERT_OK_MSG(err, "Didn't call the end tx cb that reallocates buffer one more time.");

	/* Start multi advs to check that all buffers are sent and cbs are triggered. */
	send_cb.start = seq_start_cb;
	send_cb.end = seq_end_cb;
	seq_checker = 0;

	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		net_buf_add_le32(buf[i], i);
		bt_mesh_adv_send(buf[i], &send_cb, (void *)(intptr_t)i);
		net_buf_unref(buf[i]);
	}

	err = k_sem_take(&observer_sem, K_SECONDS(10));
	ASSERT_OK_MSG(err, "Didn't call the last end tx cb.");

	PASS();
}

static void test_tx_proxy_mixin(void)
{
	static struct bt_mesh_prov prov = {
		.uuid = test_prov_uuid,
	};
	uint8_t status;
	int err;

	/* Initialize mesh stack and enable pb gatt bearer to emit beacons. */
	bt_mesh_device_setup(&prov, &comp);
	err = bt_mesh_prov_enable(BT_MESH_PROV_GATT);
	ASSERT_OK_MSG(err, "Failed to enable GATT provisioner");

	/* Let the tester to measure an interval between advertisements.
	 * The node should advertise pb gatt service with 100 msec interval.
	 */
	k_sleep(K_MSEC(1800));

	LOG_INF("Provision device under test");
	/* Provision dut and start gatt proxy beacons. */
	bt_mesh_provision(test_net_key, 0, 0, 0, adv_cfg.addr, adv_cfg.dev_key);
	/* Disable secured network beacons to exclude influence of them on proxy beaconing. */
	ASSERT_OK(bt_mesh_cfg_cli_beacon_set(0, adv_cfg.addr, BT_MESH_BEACON_DISABLED, &status));
	ASSERT_EQUAL(BT_MESH_BEACON_DISABLED, status);

	/* Let the tester to measure an interval between advertisements.
	 * The node should advertise proxy service with 1 second interval.
	 */
	k_sleep(K_MSEC(6000));

	/* Send a mesh message while advertising proxy service.
	 * Advertising the proxy service should be resumed after
	 * finishing advertising the message.
	 */
	struct net_buf *buf = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
			BT_MESH_TRANSMIT(5, 20), K_NO_WAIT);
	net_buf_add_mem(buf, txt_msg, sizeof(txt_msg));
	bt_mesh_adv_send(buf, NULL, NULL);
	k_sleep(K_MSEC(150));

	/* Let the tester to measure an interval between advertisements again. */
	k_sleep(K_MSEC(6000));

	PASS();
}

static void test_rx_proxy_mixin(void)
{
	/* (total transmit duration) / (transmit interval) */
	gatt_param.transmits = 1500 / 100;
	gatt_param.interval = 100;
	gatt_param.service = MESH_SERVICE_PROVISIONING;

	bt_init();

	/* Scan pb gatt beacons. */
	rx_gatt_beacons();

	/* Delay to provision dut */
	k_sleep(K_MSEC(1000));

	/* Scan proxy beacons. */
	/* (total transmit duration) / (transmit interval) */
	gatt_param.transmits = 5000 / 1000;
	gatt_param.interval = 1000;
	gatt_param.service = MESH_SERVICE_PROXY;
	rx_gatt_beacons();

	/* Scan adv data. */
	xmit_param.retr = 5;
	xmit_param.interval = 20;
	rx_xmit_adv();

	/* Scan proxy beacons again. */
	rx_gatt_beacons();

	PASS();
}

static void test_tx_send_order(void)
{
	struct net_buf *buf[CONFIG_BT_MESH_ADV_BUF_COUNT];
	uint8_t xmit = BT_MESH_TRANSMIT(2, 20);

	bt_init();
	adv_init();

	/* Verify sending order */
	allocate_all_array(buf, ARRAY_SIZE(buf), xmit);
	verify_adv_queue_overflow();
	send_adv_array(&buf[0], ARRAY_SIZE(buf), false);

	/* Wait for no message receive window to end. */
	ASSERT_OK_MSG(k_sem_take(&observer_sem, K_SECONDS(10)),
		      "Didn't call the last end tx cb.");

	/* Verify buffer allocation/deallocation after sending */
	allocate_all_array(buf, ARRAY_SIZE(buf), xmit);
	verify_adv_queue_overflow();
	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		net_buf_unref(buf[i]);
		buf[i] = NULL;
	}
	/* Check that it possible to add just one net buf. */
	allocate_all_array(buf, 1, xmit);

	PASS();
}

static void test_tx_reverse_order(void)
{
	struct net_buf *buf[CONFIG_BT_MESH_ADV_BUF_COUNT];
	uint8_t xmit = BT_MESH_TRANSMIT(2, 20);

	bt_init();
	adv_init();

	/* Verify reversed sending order */
	allocate_all_array(buf, ARRAY_SIZE(buf), xmit);

	send_adv_array(&buf[CONFIG_BT_MESH_ADV_BUF_COUNT - 1], ARRAY_SIZE(buf), true);

	/* Wait for no message receive window to end. */
	ASSERT_OK_MSG(k_sem_take(&observer_sem, K_SECONDS(10)),
		      "Didn't call the last end tx cb.");

	PASS();
}

static void test_tx_random_order(void)
{
	struct net_buf *buf[3];
	uint8_t xmit = BT_MESH_TRANSMIT(0, 20);

	bt_init();
	adv_init();

	/* Verify random order calls */
	num_adv_sent = ARRAY_SIZE(buf);
	previous_checker = 0xff;
	buf[0] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE(!buf[0], "Out of buffers");
	buf[1] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE(!buf[1], "Out of buffers");

	send_adv_buf(buf[0], 0, 0xff);

	buf[2] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_LOCAL_ADV,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE(!buf[2], "Out of buffers");

	send_adv_buf(buf[2], 2, 0);

	send_adv_buf(buf[1], 1, 2);

	/* Wait for no message receive window to end. */
	ASSERT_OK_MSG(k_sem_take(&observer_sem, K_SECONDS(10)),
		      "Didn't call the last end tx cb.");

	PASS();
}

static void test_rx_receive_order(void)
{
	bt_init();

	xmit_param.retr = 2;
	xmit_param.interval = 20;

	receive_order(CONFIG_BT_MESH_ADV_BUF_COUNT);

	PASS();
}

static void test_rx_random_order(void)
{
	bt_init();

	xmit_param.retr = 0;
	xmit_param.interval = 20;

	receive_order(3);

	PASS();
}

#define TEST_CASE(role, name, description)                     \
	{                                                      \
		.test_id = "adv_" #role "_" #name,             \
		.test_descr = description,                     \
		.test_pre_init_f = test_##role##_init,         \
		.test_tick_f = bt_mesh_test_timeout,           \
		.test_main_f = test_##role##_##name,           \
	}

static const struct bst_test_instance test_adv[] = {
	TEST_CASE(tx, cb_single,     "ADV: tx cb parameter checker"),
	TEST_CASE(tx, cb_multi,      "ADV: tx cb sequence checker"),
	TEST_CASE(tx, proxy_mixin,   "ADV: proxy mix-in gatt adv"),
	TEST_CASE(tx, send_order,    "ADV: tx send order"),
	TEST_CASE(tx, reverse_order, "ADV: tx reversed order"),
	TEST_CASE(tx, random_order,  "ADV: tx random order"),

	TEST_CASE(rx, xmit,          "ADV: xmit checker"),
	TEST_CASE(rx, proxy_mixin,   "ADV: proxy mix-in scanner"),
	TEST_CASE(rx, receive_order, "ADV: rx receive order"),
	TEST_CASE(rx, random_order,  "ADV: rx random order"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_adv);
	return tests;
}
