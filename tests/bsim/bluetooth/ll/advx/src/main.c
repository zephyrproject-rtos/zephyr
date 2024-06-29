/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "ll.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define HANDLE          0x0000
#define EVT_PROP_SCAN   BIT(1)
#define EVT_PROP_ANON   BIT(5)
#define EVT_PROP_TXP    BIT(6)
#define ADV_INTERVAL    0x20   /* 20 ms advertising interval */
#define ADV_WAIT_MS     10     /* 10 ms wait loop */
#define OWN_ADDR_TYPE   BT_ADDR_LE_RANDOM_ID
#define PEER_ADDR_TYPE  BT_ADDR_LE_RANDOM_ID
#define PEER_ADDR       peer_addr
#define ADV_CHAN_MAP    0x07
#define FILTER_POLICY   0x00
#define ADV_TX_PWR      NULL
#define ADV_SEC_SKIP    0
#define ADV_PHY_1M      BIT(0)
#define ADV_PHY_2M      BIT(1)
#define ADV_PHY_CODED   BIT(2)
#define ADV_SID         0x0a
#define SCAN_REQ_NOT    0

#define AD_OP           0x03
#define AD_FRAG_PREF    0x00

#define ADV_INTERVAL_PERIODIC 0x30

#define SCAN_INTERVAL   0x04
#define SCAN_WINDOW     0x04

#define FAIL(...)					\
	do {						\
		bst_result = Failed;			\
		bs_trace_error_time_line(__VA_ARGS__);	\
	} while (0)

#define PASS(...)					\
	do {						\
		bst_result = Passed;			\
		bs_trace_info_time(1, __VA_ARGS__);	\
	} while (0)

extern enum bst_result_t bst_result;

static uint8_t const own_addr_reenable[] = {0xE0, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5};
static uint8_t const own_addr[] = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5};
static uint8_t const peer_addr[] = {0xc6, 0xc7, 0xc8, 0xc9, 0xc1, 0xcb};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
	};

static uint8_t adv_data[] = {
		2, BT_DATA_FLAGS, BT_LE_AD_NO_BREDR,
	};

static uint8_t adv_data1[] = {
		2, BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
		7, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r',
	};

static uint8_t adv_data2[] = {
		2, BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR),
	};

static uint8_t sr_data[] = {
		7, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r',
	};

static uint8_t per_adv_data1[] = {
		7, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r',
	};

static uint8_t per_adv_data2[] = {
		8, BT_DATA_NAME_COMPLETE, 'Z', 'e', 'p', 'h', 'y', 'r', '1',
	};

static uint8_t per_adv_data3[] = {
		0xFF, 0xFE, 0xFD, 0xFB, 0xF7, 0xEF, 0xDF, 0xBF,
	};

static uint8_t chan_map[] = { 0x1F, 0XF1, 0x1F, 0xF1, 0x1F };

static bool volatile is_scanned, is_connected, is_disconnected;
static bool volatile connection_to_test;
static uint8_t adv_data_expected_len;
static uint8_t *adv_data_expected;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	int err;

	printk("Connected.\n");

	is_connected = true;

	err = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		printk("Disconnection failed (err %d).\n", err);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	printk("Disconnected.\n");

	is_disconnected = true;
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static bool volatile is_sent;
static uint8_t volatile num_sent_actual;

void sent_cb(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_sent_info *info)
{
	printk("%s: num_sent = %u\n", __func__, info->num_sent);

	is_sent = true;
	num_sent_actual = info->num_sent;
}

void connected_cb(struct bt_le_ext_adv *adv,
		  struct bt_le_ext_adv_connected_info *info)
{
	printk("%s\n", __func__);
}

void scanned_cb(struct bt_le_ext_adv *adv,
		struct bt_le_ext_adv_scanned_info *info)
{
	printk("%s\n", __func__);
}

struct bt_le_ext_adv_cb adv_callbacks = {
	.sent = sent_cb,
	.connected = connected_cb,
	.scanned = scanned_cb,
};

static void test_advx_main(void)
{
	struct bt_le_ext_adv_start_param ext_adv_param;
	struct bt_le_ext_adv *adv;
	uint8_t num_sent_expected;
	struct bt_data sd[1];
	uint16_t evt_prop;
	uint8_t adv_type;
	uint16_t handle;
	uint8_t phy_p;
	uint8_t phy_s;
	int err;

	printk("\n*Extended Advertising test*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_CONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		printk("Advertising failed to start (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for connection...");
	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for disconnect...");
	while (!is_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Stop advertising...");
	err = bt_le_adv_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("AD Data set...");
	handle = 0U;
	err = ll_adv_data_set(handle, sizeof(adv_data), adv_data);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Directed advertising, parameter set...");
	err = ll_adv_params_set(handle, 0, 0, BT_HCI_ADV_DIRECT_IND,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY,
				0, 0, 0, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Connectable advertising, parameter set...");
	err = ll_adv_params_set(handle, 0, ADV_INTERVAL, BT_HCI_ADV_NONCONN_IND,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY,
				0, 0, 0, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(100));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Directed advertising, parameter set...");
	err = ll_adv_params_set(handle, 0, 0, BT_HCI_ADV_DIRECT_IND,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY,
				0, 0, 0, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("AD Data set...");
	handle = 0U;
	err = ll_adv_data_set(handle, sizeof(adv_data1), adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Connectable advertising, parameter set...");
	err = ll_adv_params_set(handle, 0, ADV_INTERVAL, BT_HCI_ADV_NONCONN_IND,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY,
				0, 0, 0, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(100));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Create scannable extended advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_SCAN, &adv_callbacks,
				   &adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	/* Scannable advertiser need to have scan response data */
	printk("Set scan response data...");
	sd[0].type = BT_DATA_NAME_COMPLETE;
	sd[0].data_len = sizeof(CONFIG_BT_DEVICE_NAME) - 1;
	sd[0].data = CONFIG_BT_DEVICE_NAME;
	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, 1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scannable advertising...");
	ext_adv_param.timeout = 0;
	ext_adv_param.num_events = 0;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(500));

	printk("Stopping scannable advertising...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Removing scannable adv set...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Create connectable extended advertising set...");
	is_connected = false;
	is_disconnected = false;
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, &adv_callbacks, &adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start advertising...");
	ext_adv_param.timeout = 0;
	ext_adv_param.num_events = 0;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for connection...");
	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for disconnect...");
	while (!is_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Removing connectable adv aux set...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting non-connectable advertising...");
	err = bt_le_adv_start(BT_LE_ADV_NCONN, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Stopping advertising...");
	err = bt_le_adv_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Create connectable advertising set...");
	err = bt_le_ext_adv_create(BT_LE_ADV_CONN, &adv_callbacks, &adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start advertising using extended commands (max_events)...");
	is_sent = false;
	num_sent_actual = 0;
	num_sent_expected = 3;
	ext_adv_param.timeout = 0;
	ext_adv_param.num_events = 3;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting...");
	while (!is_sent) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if (num_sent_actual != num_sent_expected) {
		FAIL("Num sent actual = %u, expected = %u\n", num_sent_actual,
		     num_sent_expected);
	}

	k_sleep(K_MSEC(1000));

	printk("Start advertising using extended commands (duration)...");
	is_sent = false;
	num_sent_actual = 0;
	num_sent_expected = 5;
	ext_adv_param.timeout = 50;
	ext_adv_param.num_events = 0;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting...");
	while (!is_sent) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if (num_sent_actual != num_sent_expected) {
		FAIL("Num sent actual = %u, expected = %u\n", num_sent_actual,
		     num_sent_expected);
	}

	k_sleep(K_MSEC(1000));

	printk("Re-enable advertising using extended commands (max_events)...");
	is_sent = false;
	num_sent_actual = 0;
	num_sent_expected = 3;
	ext_adv_param.timeout = 0;
	ext_adv_param.num_events = 3;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(100));

	printk("Setting advertising random address before re-enabling...");
	handle = 0x0000;
	err = ll_adv_aux_random_addr_set(handle, own_addr_reenable);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Re-enabling...");
	handle = 0x0000;
	err = ll_adv_enable(handle, 1,
			    ext_adv_param.timeout,
			    ext_adv_param.num_events);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting...");
	while (!is_sent) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if (num_sent_actual != num_sent_expected) {
		FAIL("Num sent actual = %u, expected = %u\n", num_sent_actual,
		     num_sent_expected);
	}

	k_sleep(K_MSEC(1000));

	printk("Re-enable advertising using extended commands (duration)...");
	is_sent = false;
	num_sent_actual = 0;
	num_sent_expected = 5;      /* 5 advertising events with a spacing of (100 ms +
				     * random_delay of upto 10 ms) transmit in
				     * the range of 400 to 440 ms
				     */
	ext_adv_param.timeout = 50; /* Check there is atmost 5 advertising
				     * events in a timeout of 500 ms
				     */
	ext_adv_param.num_events = 0;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	/* Delay 100 ms, and the test should verify that re-enabling still
	 * results in correct num of events.
	 */
	k_sleep(K_MSEC(100));

	printk("Re-enabling...");
	handle = 0x0000;
	err = ll_adv_enable(handle, 1,
			    ext_adv_param.timeout,
			    ext_adv_param.num_events);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting...");
	while (!is_sent) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if (num_sent_actual != num_sent_expected) {
		FAIL("Num sent actual = %u, expected = %u\n", num_sent_actual,
		     num_sent_expected);
	}

	k_sleep(K_MSEC(1000));

	printk("Start advertising using extended commands (disable)...");
	ext_adv_param.timeout = 0;
	ext_adv_param.num_events = 5;
	err = bt_le_ext_adv_start(adv, &ext_adv_param);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Stopping advertising using extended commands...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Setting advertising random address...");
	handle = 0x0000;
	err = ll_adv_aux_random_addr_set(handle, own_addr);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Starting non-conn non-scan without aux 1M advertising...");
	evt_prop = EVT_PROP_TXP;
	adv_type = 0x07; /* Adv. Ext. */
	phy_p = ADV_PHY_1M;
	phy_s = ADV_PHY_2M;
	err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}

	printk("enabling...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Enabling non-conn non-scan without aux 1M advertising...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Adding data, so non-conn non-scan with aux 1M advertising...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data), (void *)adv_data);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 1...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data1), (void *)adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 2...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data2), (void *)adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Starting directed advertising...");
	const bt_addr_le_t direct_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0x11, 0x22, 0x33, 0x44, 0x55, 0xC6}
		}
	};
	const struct bt_le_adv_param adv_param = {
		.options = BT_LE_ADV_OPT_CONNECTABLE,
		.peer = &direct_addr,
	};
	err = bt_le_adv_start(&adv_param, NULL, 0, NULL, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(2000));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Add to resolving list...");
	bt_addr_le_t peer_id_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0xc6, 0xc7, 0xc8, 0xc9, 0xc1, 0xcb}
		}
	};
	uint8_t pirk[16] = {0x00, };
	uint8_t lirk[16] = {0x01, };

	err = ll_rl_add(&peer_id_addr, pirk, lirk);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enable resolving list...");
	err = ll_rl_enable(BT_HCI_ADDR_RES_ENABLE);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling extended...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Starting periodic 1M advertising...");
	err = ll_adv_sync_param_set(handle, ADV_INTERVAL_PERIODIC, 0);
	if (err) {
		goto exit;
	}

	printk("enabling periodic...");
	err = ll_adv_sync_enable(handle, BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 1...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data1), (void *)adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update advertising data 2...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data2), (void *)adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update periodic advertising data 1...");
	err = ll_adv_sync_ad_data_set(handle, AD_OP, sizeof(per_adv_data1),
				      (void *)per_adv_data1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update periodic advertising data 2...");
	err = ll_adv_sync_ad_data_set(handle, AD_OP, sizeof(per_adv_data2),
				      (void *)per_adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update periodic advertising data 3...");
	err = ll_adv_sync_ad_data_set(handle, AD_OP, sizeof(per_adv_data3),
				      (void *)per_adv_data3);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update periodic advertising back to data 2...");
	err = ll_adv_sync_ad_data_set(handle, AD_OP, sizeof(per_adv_data2),
				      (void *)per_adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Periodic Advertising Channel Map Indication...");
	err = ll_chm_update(chan_map);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling periodic...");
	err = ll_adv_sync_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("enabling periodic...");
	err = ll_adv_sync_enable(handle,
				 (BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE |
				  BT_HCI_LE_SET_PER_ADV_ENABLE_ADI));
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling extended...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Update periodic advertising data (duplicate filter)...");
	err = ll_adv_sync_ad_data_set(handle, AD_OP, sizeof(per_adv_data3),
				      (void *)per_adv_data3);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Disabling periodic...");
	err = ll_adv_sync_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Adding scan response data on non-scannable set...");
	err = ll_adv_aux_sr_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(sr_data), (void *)sr_data);
	if (err != BT_HCI_ERR_INVALID_PARAM) {
		goto exit;
	}
	printk("success.\n");

	printk("Removing adv aux set that's created and disabled ...");
	err = ll_adv_aux_set_remove(handle);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating new adv set (scannable)...");
	err = ll_adv_params_set(handle, EVT_PROP_SCAN, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Adding scan response data...");
	err = ll_adv_aux_sr_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(sr_data), (void *)sr_data);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling non-conn scan with scan response data...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Removing adv aux set that's created and disabled ...");
	err = ll_adv_aux_set_remove(handle);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Removing adv aux set that's not created ...");
	err = ll_adv_aux_set_remove(handle);
	if (err != BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating new adv set...");
	err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Update advertising data 2...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data2), (void *)adv_data2);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling adv set...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Removing adv aux set that's created and enabled  ...");
	err = ll_adv_aux_set_remove(handle);
	if (err != BT_HCI_ERR_CMD_DISALLOWED) {
		goto exit;
	}
	printk("success.\n");

	printk("Disabling adv set...");
	err = ll_adv_enable(handle, 0, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Removing adv aux set that's created and disabled  ...");
	err = ll_adv_aux_set_remove(handle);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating new adv set...");
	err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Starting periodic 1M advertising...");
	err = ll_adv_sync_param_set(handle, ADV_INTERVAL_PERIODIC, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("enabling periodic...");
	err = ll_adv_sync_enable(handle, BT_HCI_LE_SET_PER_ADV_ENABLE_ENABLE);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Trying to remove an adv set with sync enabled ...");
	err = ll_adv_aux_set_remove(handle);
	if (err != BT_HCI_ERR_CMD_DISALLOWED) {
		goto exit;
	}
	printk("success.\n");

	printk("Disabling periodic...");
	err = ll_adv_sync_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Trying to remove an adv set after sync disabled ...");
	err = ll_adv_aux_set_remove(handle);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	uint8_t num_adv_sets;
	num_adv_sets = ll_adv_aux_set_count_get();

	printk("Creating every other adv set ...");
	for (handle = 0; handle < num_adv_sets; handle += 2) {
		err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
					OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
					ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
					phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
					SCAN_REQ_NOT);
		if (err) {
			goto exit;
		}
	}
	printk("success.\n");

	printk("Clearing all adv sets...");
	err = ll_adv_aux_set_clear();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Trying to remove adv sets ...");
	for (handle = 0; handle < num_adv_sets; ++handle) {
		err = ll_adv_aux_set_remove(handle);
		if (err != BT_HCI_ERR_UNKNOWN_ADV_IDENTIFIER) {
			goto exit;
		}
	}
	printk("success.\n");

	printk("Creating one adv set ...");
	handle = 0;
	err = ll_adv_params_set(handle, evt_prop, ADV_INTERVAL, adv_type,
				OWN_ADDR_TYPE, PEER_ADDR_TYPE, PEER_ADDR,
				ADV_CHAN_MAP, FILTER_POLICY, ADV_TX_PWR,
				phy_p, ADV_SEC_SKIP, phy_s, ADV_SID,
				SCAN_REQ_NOT);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enabling adv set...");
	err = ll_adv_enable(handle, 1, 0, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Clearing all adv sets...");
	err = ll_adv_aux_set_clear();
	if (err != BT_HCI_ERR_CMD_DISALLOWED) {
		goto exit;
	}
	printk("success.\n");

	PASS("AdvX tests Passed\n");
	bs_trace_silent_exit(0);

	return;

exit:
	printk("failed (%d)\n", err);

	bst_result = Failed;
	bs_trace_silent_exit(0);
}


static bool is_reenable_addr;

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
	printk("%s: type = 0x%x, addr = %s\n", __func__, adv_type, le_addr);

	if (!is_reenable_addr &&
	    !memcmp(own_addr_reenable, addr->a.val,
		    sizeof(own_addr_reenable))) {
		is_reenable_addr = true;
	}

	if (connection_to_test) {
		struct bt_conn *conn;
		int err;

		connection_to_test = false;

		err = bt_le_scan_stop();
		if (err) {
			printk("Stop LE scan failed (err %d)\n", err);
			return;
		}

		err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
					BT_LE_CONN_PARAM_DEFAULT,
					&conn);
		if (err) {
			printk("Create conn failed (err %d)\n", err);
		} else {
			bt_conn_unref(conn);
		}
	} else if (!is_scanned) {
		char addr_str[BT_ADDR_LE_STR_LEN];

		bt_addr_le_to_str(addr, addr_str, sizeof(addr_str));
		printk("Device found: %s, type: %u, AD len: %u, RSSI %d\n",
			addr_str, adv_type, buf->len, rssi);

		if ((buf->len == adv_data_expected_len) &&
		    !memcmp(buf->data, adv_data_expected,
			    adv_data_expected_len)) {
			is_scanned = true;
		}
	}
}

static const char *phy2str(uint8_t phy)
{
	switch (phy) {
	case 0: return "No packets";
	case BT_GAP_LE_PHY_1M: return "LE 1M";
	case BT_GAP_LE_PHY_2M: return "LE 2M";
	case BT_GAP_LE_PHY_CODED: return "LE Coded";
	default: return "Unknown";
	}
}

#define NAME_LEN 30

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		memcpy(name, data->data, MIN(data->data_len, NAME_LEN - 1));
		return false;
	default:
		return true;
	}
}

static bool is_scannable;
static bool is_scan_rsp;
static bool is_periodic;
static uint8_t per_sid;
static bt_addr_le_t per_addr;
static uint8_t per_adv_evt_cnt_actual;

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
	       "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s, "
	       "Interval: 0x%04x (%u ms), SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->interval, info->interval * 5 / 4, info->sid);

	if (!is_scannable &&
	    ((info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0)) {
		is_scannable = true;
	}

	if (!is_scan_rsp &&
	    ((info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0) &&
	    ((info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0) &&
	    (strlen(name) == strlen(CONFIG_BT_DEVICE_NAME)) &&
	    (!strcmp(name, CONFIG_BT_DEVICE_NAME))) {
		is_scan_rsp = true;
	}

	if (info->interval) {
		if (!is_periodic) {
			is_periodic = true;
			per_sid = info->sid;
			bt_addr_le_copy(&per_addr, info->addr);
		} else {
			if ((per_sid == info->sid) &&
			    bt_addr_le_eq(&per_addr, info->addr)) {
				per_adv_evt_cnt_actual++;

				printk("per_adv_evt_cnt_actual %u\n",
				       per_adv_evt_cnt_actual);
			}
		}
	}
}

static bool is_scan_timeout;

static void scan_timeout(void)
{
	is_scan_timeout = true;
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
	.timeout = scan_timeout,
};

static bool volatile is_sync;
static bool volatile is_sync_report;
static bool volatile is_sync_lost;
static uint8_t volatile sync_report_len;
static uint8_t sync_report_data[251];

static void
per_adv_sync_sync_cb(struct bt_le_per_adv_sync *sync,
		     struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
	       "Interval 0x%04x (%u ms), PHY %s\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr,
	       info->interval, info->interval * 5 / 4, phy2str(info->phy));

	is_sync = true;
}

static void
per_adv_sync_terminated_cb(struct bt_le_per_adv_sync *sync,
			   const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	is_sync_lost = true;
}

static void
per_adv_sync_recv_cb(struct bt_le_per_adv_sync *sync,
		     const struct bt_le_per_adv_sync_recv_info *info,
		     struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len);

	if (!is_sync_report) {
		is_sync_report = true;
		sync_report_len = buf->len;
		memcpy(sync_report_data, buf->data, buf->len);
	}
}

static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = per_adv_sync_sync_cb,
	.term = per_adv_sync_terminated_cb,
	.recv = per_adv_sync_recv_cb
};

static void test_scanx_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_LE_SCAN_TYPE_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0004,
		.window     = 0x0004,
	};
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync = NULL;
	uint8_t per_adv_evt_cnt_expected;
	uint8_t sync_report_len_prev;
	int err;

	printk("\n*Extended Scanning test*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	connection_to_test = true;

	printk("Start scanning...");
	is_reenable_addr = false;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for connection...");
	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for disconnect...");
	while (!is_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	is_connected = false;
	is_disconnected = false;
	connection_to_test = false;

	printk("Start scanning...");
	adv_data_expected = adv_data;
	adv_data_expected_len = sizeof(adv_data);
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for advertising report, switch back from directed...");
	while (!is_scanned) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for advertising report, data update while directed...");
	adv_data_expected = adv_data1;
	adv_data_expected_len = sizeof(adv_data1);
	is_scanned = false;
	while (!is_scanned) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scanning...");
	is_scannable = false;
	is_scan_rsp = false;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for scannable advertising report...\n");
	while (!is_scannable) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for scan response advertising report...\n");
	while (!is_scan_rsp) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	/* This wait is to ensure we match with connectable advertising in the
	 * advertiser's timeline.
	 */
	k_sleep(K_MSEC(500));

	connection_to_test = true;

	printk("Waiting for connection...");
	while (!is_connected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Waiting for disconnect...");
	while (!is_disconnected) {
		k_sleep(K_MSEC(100));
	}
	printk("success.\n");

	printk("Start scanning for a duration...");
	is_scan_timeout = false;
	scan_param.interval = 0x08;
	scan_param.timeout = 100;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(scan_param.timeout * 10 + 10));

	printk("Checking for scan timeout...");
	if (!is_scan_timeout) {
		err = -EIO;
		goto exit;
	}
	printk("done.\n");

	printk("Start continuous scanning for a duration...");
	is_scan_timeout = false;
	scan_param.interval = 0x04;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(scan_param.timeout * 10 + 10));

	printk("Checking for scan timeout...");
	if (!is_scan_timeout) {
		err = -EIO;
		goto exit;
	}
	printk("done.\n");

	scan_param.timeout = 0;

	k_sleep(K_MSEC(2000));

	printk("Start scanning for Periodic Advertisements...");
	is_periodic = false;
	is_reenable_addr = false;
	per_adv_evt_cnt_actual = 0;
	per_adv_evt_cnt_expected = 3;
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Verify address update due to re-enable of advertising...");
	while (!is_reenable_addr) {
		k_sleep(K_MSEC(30));
	}
	printk("success.\n");

	printk("Waiting...");
	while (!is_periodic ||
	       (per_adv_evt_cnt_actual != per_adv_evt_cnt_expected)) {
		k_sleep(K_MSEC(ADV_WAIT_MS));
	}
	printk("done.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync 0...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = 0xf;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Check duplicate Periodic Advertising Sync create before sync "
	       "established event...");
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (!err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scanning...");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Canceling Periodic Advertising Sync 0 while scanning...");
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync 1...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = 0xf;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Canceling Periodic Advertising Sync 1 without scanning...");
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync 2...");
	is_sync = false;
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scanning...");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for sync...");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	printk("Check duplicate Periodic Advertising Sync create after sync "
	       "established event...");
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (!err) {
		goto exit;
	}
	printk("success.\n");

	printk("Deleting Periodic Advertising Sync 2...");
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync 3, test sync lost...");
	is_sync = false;
	is_sync_report = false;
	sync_report_len = 0;
	is_sync_lost = false;
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = 0;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scanning...");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for sync...");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for Periodic Advertising Report of 0 bytes...");
	while (!is_sync_report) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if (sync_report_len != 0) {
		FAIL("Incorrect Periodic Advertising Report data.");
	}

	printk("Waiting for Periodic Advertising Report of %u bytes...",
	       sizeof(per_adv_data1));
	sync_report_len_prev = sync_report_len;
	while (!is_sync_report || (sync_report_len == sync_report_len_prev)) {
		is_sync_report = false;
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if ((sync_report_len != sizeof(per_adv_data1)) ||
	    memcmp(sync_report_data, per_adv_data1, sizeof(per_adv_data1))) {
		FAIL("Incorrect Periodic Advertising Report data.");
	}

	printk("Waiting for Periodic Advertising Report of %u bytes...",
	       sizeof(per_adv_data2));
	sync_report_len_prev = sync_report_len;
	while (!is_sync_report || (sync_report_len == sync_report_len_prev)) {
		is_sync_report = false;
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if ((sync_report_len != sizeof(per_adv_data2)) ||
	    memcmp(sync_report_data, per_adv_data2, sizeof(per_adv_data2))) {
		FAIL("Incorrect Periodic Advertising Report data.");
	}

	printk("Waiting for Periodic Advertising Report of %u bytes...",
	       sizeof(per_adv_data3));
	sync_report_len_prev = sync_report_len;
	while (!is_sync_report || (sync_report_len == sync_report_len_prev)) {
		is_sync_report = false;
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if ((sync_report_len != sizeof(per_adv_data3)) ||
	    memcmp(sync_report_data, per_adv_data3, sizeof(per_adv_data3))) {
		FAIL("Incorrect Periodic Advertising Report data.");
	}

	printk("Waiting for Periodic Advertising Report of %u bytes...",
	       sizeof(per_adv_data2));
	sync_report_len_prev = sync_report_len;
	while (!is_sync_report || (sync_report_len == sync_report_len_prev)) {
		is_sync_report = false;
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if ((sync_report_len != sizeof(per_adv_data2)) ||
	    memcmp(sync_report_data, per_adv_data2, sizeof(per_adv_data2))) {
		FAIL("Incorrect Periodic Advertising Report data.");
	}

	printk("Waiting for sync loss...");
	while (!is_sync_lost) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	printk("Add to resolving list...");
	bt_addr_le_t peer_id_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {
			.val = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5}
		}
	};
	uint8_t pirk[16] = {0x01, };
	uint8_t lirk[16] = {0x00, };

	err = ll_rl_add(&peer_id_addr, pirk, lirk);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Enable resolving list...");
	err = ll_rl_enable(BT_HCI_ADDR_RES_ENABLE);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Add device to periodic advertising list...");
	err = bt_le_per_adv_list_add(&peer_id_addr, per_sid);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync 4 after sync lost...");
	is_sync = false;
	is_sync_report = false;
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options = BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST |
				    BT_LE_PER_ADV_SYNC_OPT_FILTER_DUPLICATE;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Start scanning...");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for sync...");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Waiting for Periodic Advertising Report of %u bytes...",
	       sizeof(per_adv_data3));
	sync_report_len_prev = sync_report_len;
	while (!is_sync_report || (sync_report_len == sync_report_len_prev)) {
		is_sync_report = false;
		k_sleep(K_MSEC(100));
	}
	printk("done.\n");

	if ((sync_report_len != sizeof(per_adv_data3)) ||
	    memcmp(sync_report_data, per_adv_data3, sizeof(per_adv_data3))) {
		FAIL("Incorrect Periodic Advertising Report data (%u != %u).",
		     sync_report_len, sizeof(per_adv_data3));
	}

	printk("Wait for no duplicate Periodic Advertising Report"
	       " is generated...");
	is_sync_report = false;
	k_sleep(K_MSEC(400));
	if (is_sync_report) {
		goto exit;
	}
	printk("success\n");

	printk("Deleting Periodic Advertising Sync 4...");
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	PASS("ScanX tests Passed\n");

	return;

exit:
	printk("failed (%d)\n", err);

	bst_result = Failed;
	bs_trace_silent_exit(0);
}

static void test_advx_init(void)
{
	bst_ticker_set_next_tick_absolute(30e6);
	bst_result = In_progress;
}

static void test_advx_tick(bs_time_t HW_device_time)
{
	bst_result = Failed;
	bs_trace_error_line("Test advx/scanx finished.\n");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "advx",
		.test_descr = "Extended Advertising",
		.test_pre_init_f = test_advx_init,
		.test_tick_f = test_advx_tick,
		.test_main_f = test_advx_main
	},
	{
		.test_id = "scanx",
		.test_descr = "Extended scanning",
		.test_pre_init_f = test_advx_init,
		.test_tick_f = test_advx_tick,
		.test_main_f = test_scanx_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_advx_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_advx_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
