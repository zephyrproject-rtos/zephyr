/* main.c - Application main entry point */

/*
 * Copyright (c) 2015-2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "ll.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

#define HANDLE          0x0000
#define EVT_PROP_ANON   BIT(5)
#define EVT_PROP_TXP    BIT(6)
#define ADV_INTERVAL    0x20
#define OWN_ADDR_TYPE   1
#define PEER_ADDR_TYPE  0
#define PEER_ADDR       NULL
#define ADV_CHAN_MAP    0x07
#define FILTER_POLICY   0x00
#define ADV_TX_PWR      NULL
#define ADV_SEC_SKIP    0
#define ADV_PHY_1M      BIT(0)
#define ADV_PHY_2M      BIT(1)
#define ADV_PHY_CODED   BIT(2)
#define ADV_SID         0
#define SCAN_REQ_NOT    0

#define AD_OP           0x03
#define AD_FRAG_PREF    0x00

#define ADV_INTERVAL_PERIODIC 0x30

#define SCAN_INTERVAL   0x04
#define SCAN_WINDOW     0x04

extern enum bst_result_t bst_result;

static uint8_t const own_addr[] = {0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5};

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

static void test_advx_main(void)
{
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

	printk("Setting advertising random address...");
	handle = 0x0000;
	err = ll_adv_aux_random_addr_set(handle, own_addr);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("Starting non-conn non-scan without aux 1M advertising...");
	evt_prop = EVT_PROP_TXP;
	adv_type = 0x05; /* Adv. Ext. */
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
	err = ll_adv_enable(handle, 1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(400));

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting non-conn non-scan with aux 1M advertising...");
	err = ll_adv_aux_ad_data_set(handle, AD_OP, AD_FRAG_PREF,
				     sizeof(adv_data), (void *)adv_data);
	if (err) {
		goto exit;
	}

	printk("enabling...");
	err = ll_adv_enable(handle, 1);
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

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Starting periodic 1M advertising...");
	err = ll_adv_sync_param_set(handle, ADV_INTERVAL_PERIODIC, 0);
	if (err) {
		goto exit;
	}

	printk("enabling periodic...");
	err = ll_adv_sync_enable(handle, 1);
	if (err) {
		goto exit;
	}

	printk("enabling extended...");
	err = ll_adv_enable(handle, 1);
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

	printk("Disabling...");
	err = ll_adv_enable(handle, 0);
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

	bst_result = Passed;
	bs_trace_silent_exit(0);

	return;

exit:
	printk("failed (%d)\n", err);

	bst_result = Failed;
	bs_trace_silent_exit(0);
}

static void scan_cb(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		    struct net_buf_simple *buf)
{
	printk("%s: type = 0x%x.\n", __func__, adv_type);
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

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[NAME_LEN];

	(void)memset(name, 0, sizeof(name));

	bt_data_parse(buf, data_cb, name);

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));
	printk("[DEVICE]: %s, AD evt type %u, Tx Pwr: %i, RSSI %i %s "
	       "C:%u S:%u D:%u SR:%u E:%u Prim: %s, Secn: %s "
	       "SID: %u\n",
	       le_addr, info->adv_type, info->tx_power, info->rssi, name,
	       (info->adv_props & BT_GAP_ADV_PROP_CONNECTABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCANNABLE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_SCAN_RESPONSE) != 0,
	       (info->adv_props & BT_GAP_ADV_PROP_EXT_ADV) != 0,
	       phy2str(info->primary_phy), phy2str(info->secondary_phy),
	       info->sid);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void test_scanx_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type       = BT_HCI_LE_SCAN_ACTIVE,
		.options    = BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0004,
		.window     = 0x0004,
	};
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

	printk("Start scanning...");
	err = bt_le_scan_start(&scan_param, scan_cb);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_SECONDS(5));

#if TEST_LOW_LEVEL
	uint8_t type = (BIT(0) << 1) | 0x01; /* 1M PHY and active scanning */

	printk("Setting scan parameters...");
	err = ll_scan_params_set(type, SCAN_INTERVAL, SCAN_WINDOW,
				 OWN_ADDR_TYPE, FILTER_POLICY);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	printk("enabling...");
	err = ll_scan_enable(1);
	if (err) {
		goto exit;
	}
	printk("success.\n");

	k_sleep(K_SECONDS(5));

	printk("Disabling...");
	err = ll_scan_enable(0);
	if (err) {
		goto exit;
	}
	printk("success.\n");
#endif

	bst_result = Passed;

	return;

exit:
	printk("failed (%d)\n", err);

	bst_result = Failed;
	bs_trace_silent_exit(0);
}

static void test_advx_init(void)
{
	bst_ticker_set_next_tick_absolute(10e6);
	bst_result = In_progress;
}

static void test_advx_tick(bs_time_t HW_device_time)
{

	bst_result = Failed;
	bs_trace_error_line("Test advx finished.\n");

}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "advx",
		.test_descr = "Extended Advertising",
		.test_post_init_f = test_advx_init,
		.test_tick_f = test_advx_tick,
		.test_main_f = test_advx_main
	},
	{
		.test_id = "scanx",
		.test_descr = "Extended scanning",
		.test_post_init_f = test_advx_init,
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

void main(void)
{
	bst_main();
}
