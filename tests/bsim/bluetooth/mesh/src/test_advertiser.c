/*
 * Copyright (c) 2022 Nordic Semiconductor
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/hci.h>
#include "mesh_test.h"
#include "mesh/net.h"
#include "mesh/mesh.h"
#include "mesh/foundation.h"
#include "gatt_common.h"

#define LOG_MODULE_NAME test_adv

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(LOG_MODULE_NAME, LOG_LEVEL_INF);

#define WAIT_TIME 60 /*seconds*/

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

static void allocate_all_array(struct bt_mesh_adv **adv, size_t num_adv, uint8_t xmit)
{
	for (int i = 0; i < num_adv; i++) {
		*adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
					  xmit, K_NO_WAIT);

		ASSERT_FALSE_MSG(!*adv, "Out of advs\n");
		adv++;
	}
}

static void verify_adv_queue_overflow(void)
{
	struct bt_mesh_adv *dummy_adv;

	/* Verity Queue overflow */
	dummy_adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				       BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_TRUE_MSG(!dummy_adv, "Unexpected extra adv\n");
}

static bool check_delta_time(uint8_t transmit, uint64_t interval)
{
	static int cnt;
	static int64_t timestamp;

	if (cnt) {
		int64_t delta = k_uptime_delta(&timestamp);

		LOG_INF("rx: cnt(%d) delta(%dms) interval(%ums)",
			cnt, (int32_t)delta, (uint32_t)interval);

		ASSERT_TRUE(delta >= (interval - 5) &&
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
	struct bt_mesh_adv *adv = (struct bt_mesh_adv *)cb_data;

	ASSERT_EQUAL(0, err);
	adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
			BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_FALSE_MSG(!adv, "Out of advs\n");

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

static void gatt_scan_cb(const bt_addr_le_t *addr, int8_t rssi,
			  uint8_t adv_type, struct net_buf_simple *buf)
{
	if (adv_type != BT_GAP_ADV_TYPE_ADV_IND) {
		return;
	}

	bt_mesh_test_parse_mesh_gatt_preamble(buf);

	if (gatt_param.service == MESH_SERVICE_PROVISIONING) {
		bt_mesh_test_parse_mesh_pb_gatt_service(buf);
	} else {
		bt_mesh_test_parse_mesh_proxy_service(buf);
	}

	LOG_INF("rx: %s", txt_msg);

	if (check_delta_time(gatt_param.transmits, gatt_param.interval)) {
		LOG_INF("rx completed. stop observer.");
		k_sem_give(&observer_sem);
	}
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

static void send_order_start_cb(uint16_t duration, int err, void *user_data)
{
	struct bt_mesh_adv *adv = (struct bt_mesh_adv *)user_data;

	ASSERT_OK_MSG(err, "Failed adv start cb err (%d)", err);
	ASSERT_EQUAL(2, adv->b.len);

	uint8_t current = adv->b.data[0];
	uint8_t previous = adv->b.data[1];

	LOG_INF("tx start: current(%d) previous(%d)", current, previous);

	ASSERT_EQUAL(previous_checker, previous);
	previous_checker = current;
}

static void send_order_end_cb(int err, void *user_data)
{
	ASSERT_OK_MSG(err, "Failed adv start cb err (%d)", err);
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
	previous_checker = 0xff;
	for (int i = 0; i < expect_adv; i++) {
		ASSERT_OK(bt_mesh_test_wait_for_packet(receive_order_scan_cb, &observer_sem, 10));
	}
}

static void send_adv_buf(struct bt_mesh_adv *adv, uint8_t curr, uint8_t prev)
{
	send_cb.start = send_order_start_cb;
	send_cb.end = send_order_end_cb;

	(void)net_buf_simple_add_u8(&adv->b, curr);
	(void)net_buf_simple_add_u8(&adv->b, prev);

	bt_mesh_adv_send(adv, &send_cb, adv);
	bt_mesh_adv_unref(adv);
}

static void send_adv_array(struct bt_mesh_adv **adv, size_t num_buf, bool reverse)
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
		send_adv_buf(*adv, (uint8_t)i, previous);
		previous = (uint8_t)i;
		if (!reverse) {
			adv++;
			i++;
		} else {
			adv--;
			i--;
		}
	}
}

static void test_tx_cb_single(void)
{
	struct bt_mesh_adv *adv;
	int err;

	bt_init();
	adv_init();

	adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
			BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_FALSE_MSG(!adv, "Out of advs\n");

	send_cb.start = single_start_cb;
	send_cb.end = single_end_cb;

	net_buf_simple_add_mem(&adv->b, txt_msg, sizeof(txt_msg));
	seq_checker = 0;
	tx_timestamp = k_uptime_get();
	bt_mesh_adv_send(adv, &send_cb, (void *)cb_msg);
	bt_mesh_adv_unref(adv);

	err = k_sem_take(&observer_sem, K_SECONDS(1));
	ASSERT_OK_MSG(err, "Didn't call end tx cb.");

	PASS();
}

static void test_rx_xmit(void)
{
	xmit_param.retr = 2;
	xmit_param.interval = 20;

	bt_init();
	ASSERT_OK(bt_mesh_test_wait_for_packet(xmit_scan_cb, &observer_sem, 20));

	PASS();
}

static void test_tx_cb_multi(void)
{
	struct bt_mesh_adv *adv[CONFIG_BT_MESH_ADV_BUF_COUNT];
	int err;

	bt_init();
	adv_init();

	/* Allocate all network advs. */
	allocate_all_array(adv, ARRAY_SIZE(adv), BT_MESH_TRANSMIT(2, 20));

	/* Start single adv to reallocate one network adv in callback.
	 * Check that the adv is freed before cb is triggered.
	 */
	send_cb.start = NULL;
	send_cb.end = realloc_end_cb;
	net_buf_simple_add_mem(&(adv[0]->b), txt_msg, sizeof(txt_msg));

	bt_mesh_adv_send(adv[0], &send_cb, adv[0]);
	bt_mesh_adv_unref(adv[0]);

	err = k_sem_take(&observer_sem, K_SECONDS(1));
	ASSERT_OK_MSG(err, "Didn't call the end tx cb that reallocates adv one more time.");

	/* Start multi advs to check that all advs are sent and cbs are triggered. */
	send_cb.start = seq_start_cb;
	send_cb.end = seq_end_cb;
	seq_checker = 0;

	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		net_buf_simple_add_le32(&(adv[i]->b), i);
		bt_mesh_adv_send(adv[i], &send_cb, (void *)(intptr_t)i);
		bt_mesh_adv_unref(adv[i]);
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
	struct bt_mesh_adv *adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
			BT_MESH_TRANSMIT(5, 20), K_NO_WAIT);
	net_buf_simple_add_mem(&adv->b, txt_msg, sizeof(txt_msg));
	bt_mesh_adv_send(adv, NULL, NULL);
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
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &observer_sem, 20));

	/* Delay to provision dut */
	k_sleep(K_MSEC(1000));

	/* Scan proxy beacons. */
	/* (total transmit duration) / (transmit interval) */
	gatt_param.transmits = 5000 / 1000;
	gatt_param.interval = 1000;
	gatt_param.service = MESH_SERVICE_PROXY;
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &observer_sem, 20));

	/* Scan adv data. */
	xmit_param.retr = 5;
	xmit_param.interval = 20;
	ASSERT_OK(bt_mesh_test_wait_for_packet(xmit_scan_cb, &observer_sem, 20));

	/* Scan proxy beacons again. */
	ASSERT_OK(bt_mesh_test_wait_for_packet(gatt_scan_cb, &observer_sem, 20));

	PASS();
}

static void test_tx_send_order(void)
{
	struct bt_mesh_adv *adv[CONFIG_BT_MESH_ADV_BUF_COUNT];
	uint8_t xmit = BT_MESH_TRANSMIT(2, 20);

	bt_init();
	adv_init();

	/* Verify sending order */
	allocate_all_array(adv, ARRAY_SIZE(adv), xmit);
	verify_adv_queue_overflow();
	send_adv_array(&adv[0], ARRAY_SIZE(adv), false);

	/* Wait for no message receive window to end. */
	ASSERT_OK_MSG(k_sem_take(&observer_sem, K_SECONDS(10)),
		      "Didn't call the last end tx cb.");

	/* Verify adv allocation/deallocation after sending */
	allocate_all_array(adv, ARRAY_SIZE(adv), xmit);
	verify_adv_queue_overflow();
	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		bt_mesh_adv_unref(adv[i]);
		adv[i] = NULL;
	}
	/* Check that it possible to add just one net adv. */
	allocate_all_array(adv, 1, xmit);

	PASS();
}

static void test_tx_reverse_order(void)
{
	struct bt_mesh_adv *adv[CONFIG_BT_MESH_ADV_BUF_COUNT];
	uint8_t xmit = BT_MESH_TRANSMIT(2, 20);

	bt_init();
	adv_init();

	/* Verify reversed sending order */
	allocate_all_array(adv, ARRAY_SIZE(adv), xmit);

	send_adv_array(&adv[CONFIG_BT_MESH_ADV_BUF_COUNT - 1], ARRAY_SIZE(adv), true);

	/* Wait for no message receive window to end. */
	ASSERT_OK_MSG(k_sem_take(&observer_sem, K_SECONDS(10)),
		      "Didn't call the last end tx cb.");

	PASS();
}

static void test_tx_random_order(void)
{
	struct bt_mesh_adv *adv[3];
	uint8_t xmit = BT_MESH_TRANSMIT(0, 20);

	bt_init();
	adv_init();

	/* Verify random order calls */
	num_adv_sent = ARRAY_SIZE(adv);
	previous_checker = 0xff;
	adv[0] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE_MSG(!adv[0], "Out of advs\n");
	adv[1] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE_MSG(!adv[1], "Out of advs\n");

	send_adv_buf(adv[0], 0, 0xff);

	adv[2] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				    xmit, K_NO_WAIT);
	ASSERT_FALSE_MSG(!adv[2], "Out of advs\n");

	send_adv_buf(adv[2], 2, 0);

	send_adv_buf(adv[1], 1, 2);

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

static void adv_suspend(void)
{
	atomic_set_bit(bt_mesh.flags, BT_MESH_SUSPENDED);

	ASSERT_OK_MSG(bt_mesh_adv_disable(), "Failed to disable advertiser sync");
}

static void adv_resume(void)
{
	atomic_clear_bit(bt_mesh.flags, BT_MESH_SUSPENDED);

	if (!IS_ENABLED(CONFIG_BT_EXT_ADV)) {
		bt_mesh_adv_init();
	}

	ASSERT_OK_MSG(bt_mesh_adv_enable(), "Failed to enable advertiser");
}

static void adv_disable_work_handler(struct k_work *work)
{
	adv_suspend();
}

static K_WORK_DEFINE(adv_disable_work, adv_disable_work_handler);

struct adv_suspend_ctx {
	bool suspend;
	int instance_idx;
};

static K_SEM_DEFINE(adv_sent_sem, 0, 1);
static K_SEM_DEFINE(adv_suspended_sem, 0, 1);

static void adv_send_end(int err, void *cb_data)
{
	struct adv_suspend_ctx *adv_data = cb_data;

	LOG_DBG("end(): err (%d), suspend (%d), i (%d)", err, adv_data->suspend,
		adv_data->instance_idx);

	ASSERT_EQUAL(err, 0);

	if (adv_data->suspend) {
		/* When suspending, the end callback will be called only for the first adv, because
		 * it was already scheduled.
		 */
		ASSERT_EQUAL(adv_data->instance_idx, 0);
	} else {
		if (adv_data->instance_idx == CONFIG_BT_MESH_ADV_BUF_COUNT - 1) {
			k_sem_give(&adv_sent_sem);
		}
	}
}

static void adv_send_start(uint16_t duration, int err, void *cb_data)
{
	struct adv_suspend_ctx *adv_data = cb_data;

	LOG_DBG("start(): err (%d), suspend (%d), i (%d)", err, adv_data->suspend,
		adv_data->instance_idx);

	if (adv_data->suspend) {
		if (adv_data->instance_idx == 0) {
			ASSERT_EQUAL(err, 0);
			k_work_submit(&adv_disable_work);
		} else {
			/* For the advs that were pushed to the mesh advertiser by calling
			 * `bt_mesh_adv_send` function but not sent to the host, the start callback
			 * shall be called with -ENODEV.
			 */
			ASSERT_EQUAL(err, -ENODEV);
		}

		if (adv_data->instance_idx == CONFIG_BT_MESH_ADV_BUF_COUNT - 1) {
			k_sem_give(&adv_suspended_sem);
		}
	} else {
		ASSERT_EQUAL(err, 0);
	}
}

static void adv_create_and_send(bool suspend, uint8_t first_byte, struct adv_suspend_ctx *adv_data)
{
	struct bt_mesh_adv *advs[CONFIG_BT_MESH_ADV_BUF_COUNT];
	static const struct bt_mesh_send_cb send_cb = {
		.start = adv_send_start,
		.end = adv_send_end,
	};

	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		adv_data[i].suspend = suspend;
		adv_data[i].instance_idx = i;

		advs[i] = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
					    BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
		ASSERT_FALSE_MSG(!advs[i], "Out of advs\n");

		net_buf_simple_add_u8(&advs[i]->b, first_byte);
		net_buf_simple_add_u8(&advs[i]->b, i);
	}

	for (int i = 0; i < CONFIG_BT_MESH_ADV_BUF_COUNT; i++) {
		bt_mesh_adv_send(advs[i], &send_cb, &adv_data[i]);
		bt_mesh_adv_unref(advs[i]);
	}
}

static void test_tx_disable(void)
{
	struct adv_suspend_ctx adv_data[CONFIG_BT_MESH_ADV_BUF_COUNT];
	struct bt_mesh_adv *extra_adv;
	int err;

	bt_init();
	adv_init();

	/* Fill up the adv pool and suspend the advertiser in the first start callback call. */
	adv_create_and_send(true, 0xAA, adv_data);

	err = k_sem_take(&adv_suspended_sem, K_SECONDS(10));
	ASSERT_OK_MSG(err, "Not all advs were sent");

	extra_adv = bt_mesh_adv_create(BT_MESH_ADV_DATA, BT_MESH_ADV_TAG_LOCAL,
				       BT_MESH_TRANSMIT(2, 20), K_NO_WAIT);
	ASSERT_TRUE_MSG(!extra_adv, "Created adv while suspended");

	adv_resume();

	/* Fill up the adv pool and suspend the advertiser and let it send all advs. */
	adv_create_and_send(false, 0xBB, adv_data);

	err = k_sem_take(&adv_sent_sem, K_SECONDS(10));
	ASSERT_OK_MSG(err, "Not all advs were sent");

	PASS();
}

static void suspended_adv_scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
				  struct net_buf_simple *buf)
{
	uint8_t length;
	uint8_t type;
	uint8_t pdu;

	length = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(buf->len, length);
	ASSERT_EQUAL(length, sizeof(uint8_t) * 3);
	type = net_buf_simple_pull_u8(buf);
	ASSERT_EQUAL(BT_DATA_MESH_MESSAGE, type);

	pdu = net_buf_simple_pull_u8(buf);
	if (pdu == 0xAA) {
		pdu = net_buf_simple_pull_u8(buf);

		/* Because the advertiser is stopped after the advertisement has been passed to the
		 * host, the controller could already start sending the message. Therefore, if the
		 * tester receives an advertisement with the first byte as 0xAA, the second byte can
		 * only be 0x00. This applies to both advertisers.
		 */
		ASSERT_EQUAL(0, pdu);
	}
}

static void test_rx_disable(void)
{
	int err;

	bt_init();

	/* It is sufficient to check that the advertiser didn't sent PDUs which the end callback was
	 * not called for.
	 */
	err = bt_mesh_test_wait_for_packet(suspended_adv_scan_cb, &observer_sem, 20);
	/* The error will always be -ETIMEDOUT as the semaphore is never given in the callback. */
	ASSERT_EQUAL(-ETIMEDOUT, err);

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
	TEST_CASE(tx, disable,       "ADV: test suspending/resuming advertiser"),

	TEST_CASE(rx, xmit,          "ADV: xmit checker"),
	TEST_CASE(rx, proxy_mixin,   "ADV: proxy mix-in scanner"),
	TEST_CASE(rx, receive_order, "ADV: rx receive order"),
	TEST_CASE(rx, random_order,  "ADV: rx random order"),
	TEST_CASE(rx, disable,       "ADV: rx adv from resumed advertiser"),

	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_install(struct bst_test_list *tests)
{
	tests = bst_add_tests(tests, test_adv);
	return tests;
}
