/* main.c - Application main entry point */

/*
 * Copyright (c) 2020 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/types.h>
#include <sys/printk.h>
#include <sys/util.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/hci.h>

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

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

/* TODO: The BIG/BIS host API is not yet merged, so forcibly use the controller
 * interface instead of the relying on HCI. Change to use host API once merged.
 */
#define USE_HOST_API 0

#if !IS_ENABLED(USE_HOST_API)
#include "ll.h"
#include "subsys/bluetooth/host/hci_core.h"
#endif /* !USE_HOST_API */

static uint8_t mfg_data1[] = { 0xff, 0xff, 0x01, 0x02, 0x03, 0x04 };
static uint8_t mfg_data2[] = { 0xff, 0xff, 0x05 };

static const struct bt_data per_ad_data1[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data1, 6),
};

static const struct bt_data per_ad_data2[] = {
	BT_DATA(BT_DATA_MANUFACTURER_DATA, mfg_data2, 3),
};

static void test_iso_main(void)
{
	struct bt_le_ext_adv *adv;
	int err;
	uint8_t bis_count = 1; /* TODO: Add support for multiple BIS per BIG */

	printk("\n*ISO broadcast test*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Create advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN_NAME, NULL, &adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
		return;
	}
	printk("success.\n");


	printk("Setting Periodic Advertising parameters...");
	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		FAIL("Failed to set periodic advertising parameters (err %d)\n",
		     err);
		return;
	}
	printk("success.\n");

	printk("Enable Periodic Advertising...");
	err = bt_le_per_adv_start(adv);
	if (err) {
		FAIL("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Start extended advertising...");
	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

#if !IS_ENABLED(USE_HOST_API)
	uint8_t big_handle = 0;
	uint8_t adv_handle;
	uint32_t sdu_interval = 0x10000; /*us */
	uint16_t max_sdu = 0x10;
	uint16_t max_latency = 0x0A;
	uint8_t rtn = 0;
	uint8_t phy = 0;
	uint8_t packing = 0;
	uint8_t framing = 0;
	uint8_t encryption = 0;
	uint8_t bcode[16] = { 0 };

	/* Assume that index == handle */
	adv_handle = bt_le_ext_adv_get_index(adv);

	printk("Creating BIG...");
	err = ll_big_create(big_handle, adv_handle, bis_count, sdu_interval, max_sdu,
			    max_latency, rtn, phy, packing, framing, encryption,
			    bcode);
	if (err) {
		FAIL("Could not create BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

	printk("Update periodic advertising data 1...");
	err = bt_le_per_adv_set_data(adv, per_ad_data1,
				     ARRAY_SIZE(per_ad_data1));
	if (err) {
		FAIL("Failed to update periodic advertising data 1 (%d).\n",
		     err);
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

	printk("Update periodic advertising data 2...");
	err = bt_le_per_adv_set_data(adv, per_ad_data2,
				     ARRAY_SIZE(per_ad_data2));
	if (err) {
		FAIL("Failed to update periodic advertising data 2 (%d).\n",
		     err);
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));

	printk("Terminating BIG...");
	err = ll_big_terminate(big_handle, BT_HCI_ERR_LOCALHOST_TERM_CONN);
	if (err) {
		FAIL("Could not terminate BIG: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));
#endif /* !USE_HOST_API */


	PASS("Iso tests Passed\n");

	return;
}


static bool is_periodic;
static uint8_t per_sid;
static bt_addr_le_t per_addr;
static bool volatile is_sync;

static void pa_sync_cb(struct bt_le_per_adv_sync *sync,
		     struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	is_sync = true;
}

static void pa_terminated_cb(struct bt_le_per_adv_sync *sync,
			     const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr);

	FAIL("PA terminated unexpectedly\n");
}

static void pa_recv_cb(struct bt_le_per_adv_sync *sync,
		       const struct bt_le_per_adv_sync_recv_info *info,
		       struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s, tx_power %i, "
	       "RSSI %i, CTE %u, data length %u\n",
	       bt_le_per_adv_sync_get_index(sync), le_addr, info->tx_power,
	       info->rssi, info->cte_type, buf->len);
}

static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = pa_sync_cb,
	.term = pa_terminated_cb,
	.recv = pa_recv_cb
};

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

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	char le_addr[BT_ADDR_LE_STR_LEN];
	char name[30];

	/* We only care for scan results until we find the SID */
	if (per_sid) {
		return;
	}

	(void)memset(name, 0, sizeof(name));

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

	if (info->interval) {
		if (!is_periodic) {
			is_periodic = true;
			per_sid = info->sid;
			bt_addr_le_copy(&per_addr, info->addr);
		}
	}
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void test_iso_recv_main(void)
{
	int err;

	printk("\n*ISO broadcast test*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register...");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	printk("Start scanning...");
	is_periodic = false;
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, NULL);
	if (err) {
		FAIL("Could not start scan: %d\n", err);
		return;
	}
	printk("success.\n");

	struct bt_le_per_adv_sync *sync;
	uint8_t bis_count = 1; /* TODO: Add support for multiple BIS per BIG */
	uint8_t bis_handle = 0;
	struct bt_le_per_adv_sync_param sync_create_param = { 0 };

	while (!is_periodic) {
		k_sleep(K_MSEC(100));
	}
	printk("PA SID found %u\n", per_sid);

	printk("Stop scanning...");
	err = bt_le_scan_stop();
	if (err) {
		FAIL("Could not stop scan: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Creating Periodic Advertising Sync...");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.sid = per_sid;
	sync_create_param.timeout = 0xa;
	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		FAIL("Could not create sync: %d\n", err);
		return;
	}
	printk("success.\n");

	/* TODO: Enable when advertiser is added */
	printk("Waiting for sync...");
	while (!is_sync) {
		k_sleep(K_MSEC(100));
	}

#if !IS_ENABLED(USE_HOST_API)
	uint8_t big_handle = 0;
	uint8_t mse = 0;
	uint8_t encryption = 0;
	uint8_t bcode[16] = { 0 };
	uint16_t sync_timeout = 0;
	struct node_rx_hdr *node_rx;

	printk("Creating BIG...");
	err = ll_big_sync_create(big_handle, sync->handle, encryption, bcode,
				 mse, sync_timeout, bis_count, &bis_handle);
	if (err) {
		FAIL("Could not create BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(15000));

	printk("Terminating BIG...");
	err = ll_big_sync_terminate(big_handle, (void **)&node_rx);
	if (err) {
		FAIL("Could not terminate BIG sync: %d\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(5000));
#endif /* !USE_HOST_API */

	PASS("ISO recv test Passed\n");

	return;
}

static void test_iso_init(void)
{
	bst_ticker_set_next_tick_absolute(30e6);
	bst_result = In_progress;
}

static void test_iso_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after seconds)\n");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "broadcast",
		.test_descr = "ISO broadcast",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_main
	},
	{
		.test_id = "receive",
		.test_descr = "ISO receive",
		.test_post_init_f = test_iso_init,
		.test_tick_f = test_iso_tick,
		.test_main_f = test_iso_recv_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_iso_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_iso_install,
	NULL
};

void main(void)
{
	bst_main();
}
