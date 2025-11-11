/*
 * Copyright (c) 2024 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/sys/byteorder.h>
#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>
#include <zephyr/types.h>
#include <stddef.h>
#include <errno.h>
#include <zephyr/kernel.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/iso.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/sys/byteorder.h>

#include "subsys/bluetooth/host/hci_core.h"
#include "subsys/bluetooth/controller/include/ll.h"
#include "subsys/bluetooth/controller/util/memq.h"
#include "subsys/bluetooth/controller/ll_sw/lll.h"

#include "bs_types.h"
#include "bs_tracing.h"
#include "time_machine.h"
#include "bstests.h"

static struct bt_conn *default_conn;

extern enum bst_result_t bst_result;

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

static K_SEM_DEFINE(sem_is_sync, 0, 1);
static K_SEM_DEFINE(sem_is_conn, 0, 1);

/* Set timeout to 20s */
#define K_SEM_TIMEOUT    K_MSEC(20000)

struct bt_le_per_adv_sync *default_sync;

static void connected(struct bt_conn *conn, uint8_t conn_err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (conn_err) {
		FAIL("Failed to connect to %s (%u)\n", addr, conn_err);
		return;
	}
	printk("Connected: %s\n", addr);

	k_sem_give(&sem_is_conn);
}

static bool eir_found(struct bt_data *data, void *user_data)
{
	bt_addr_le_t *addr = user_data;
	int i;

	printk("[AD]: %u data_len %u\n", data->type, data->data_len);

	switch (data->type) {
	case BT_DATA_UUID16_SOME:
	case BT_DATA_UUID16_ALL:
		if (data->data_len % sizeof(uint16_t) != 0U) {
			FAIL("AD malformed\n");
			return true;
		}

		for (i = 0; i < data->data_len; i += sizeof(uint16_t)) {
			const struct bt_uuid *uuid;
			struct bt_le_conn_param *param;
			uint16_t u16;
			int err;

			memcpy(&u16, &data->data[i], sizeof(u16));
			uuid = BT_UUID_DECLARE_16(sys_le16_to_cpu(u16));
			if (bt_uuid_cmp(uuid, BT_UUID_HRS)) {
				continue;
			}

			err = bt_le_scan_stop();
			if (err) {
				FAIL("Stop LE scan failed (err %d)\n", err);
				continue;
			}

			param = BT_LE_CONN_PARAM_DEFAULT;
			err = bt_conn_le_create(addr, BT_CONN_LE_CREATE_CONN,
						param, &default_conn);
			if (err) {
				printk("Create conn failed (err %d)\n", err);
			}

			return false;
		}
	}

	return true;
}

static void device_found(const bt_addr_le_t *addr, int8_t rssi, uint8_t type,
		struct net_buf_simple *ad)
{
	char dev[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(addr, dev, sizeof(dev));
	printk("[DEVICE]: %s, AD evt type %u, AD data len %u, RSSI %i\n",
			dev, type, ad->len, rssi);

	/* We're only interested in connectable events */
	if (type == BT_GAP_ADV_TYPE_ADV_IND ||
		type == BT_GAP_ADV_TYPE_ADV_DIRECT_IND) {
		bt_data_parse(ad, eir_found, (void *)addr);
	}
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

	if (default_conn != conn) {
		return;
	}

	bt_conn_unref(default_conn);
	default_conn = NULL;
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};


static void setup_ext_adv(struct bt_le_ext_adv **adv)
{
	int err;

	printk("Create advertising set...");
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_NCONN, NULL, adv);
	if (err) {
		FAIL("Failed to create advertising set (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Setting Periodic Advertising parameters...");
	err = bt_le_per_adv_set_param(*adv, BT_LE_PER_ADV_DEFAULT);
	if (err) {
		FAIL("Failed to set periodic advertising parameters (err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	printk("Enable Periodic Advertising...");
	err = bt_le_per_adv_start(*adv);
	if (err) {
		FAIL("Failed to enable periodic advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Start extended advertising...");
	err = bt_le_ext_adv_start(*adv, BT_LE_EXT_ADV_START_DEFAULT);
	if (err) {
		printk("Failed to start extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");
}

static void teardown_ext_adv(struct bt_le_ext_adv *adv)
{
	int err;

	printk("Stop Periodic Advertising...");
	err = bt_le_per_adv_stop(adv);
	if (err) {
		FAIL("Failed to stop periodic advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Stop Extended Advertising...");
	err = bt_le_ext_adv_stop(adv);
	if (err) {
		FAIL("Failed to stop extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Deleting Extended Advertising...");
	err = bt_le_ext_adv_delete(adv);
	if (err) {
		FAIL("Failed to delete extended advertising (err %d)\n", err);
		return;
	}
	printk("success.\n");
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

static void pa_sync_cb(struct bt_le_per_adv_sync *sync,
			 struct bt_le_per_adv_sync_synced_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	default_sync = sync;

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s synced, "
		   "Interval 0x%04x (%u ms), PHY %s\n",
		   bt_le_per_adv_sync_get_index(sync), le_addr,
		   info->interval, info->interval * 5 / 4, phy2str(info->phy));

	k_sem_give(&sem_is_sync);

	printk("Stop scanning\n");
	bt_le_scan_stop();
	printk("success.\n");
}

static void pa_terminated_cb(struct bt_le_per_adv_sync *sync,
				 const struct bt_le_per_adv_sync_term_info *info)
{
	char le_addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(info->addr, le_addr, sizeof(le_addr));

	printk("PER_ADV_SYNC[%u]: [DEVICE]: %s sync terminated\n",
		   bt_le_per_adv_sync_get_index(sync), le_addr);
}

static void
pa_state_changed_cb(struct bt_le_per_adv_sync *sync,
			const struct bt_le_per_adv_sync_state_info *info)
{
	printk("PER_ADV_SYNC[%u]: state changed, receive %s.\n",
		   bt_le_per_adv_sync_get_index(sync),
		   info->recv_enabled ? "enabled" : "disabled");
}


static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = pa_sync_cb,
	.term = pa_terminated_cb,
	.state_changed = pa_state_changed_cb,
};

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL,
			  BT_UUID_16_ENCODE(BT_UUID_HRS_VAL),
			  BT_UUID_16_ENCODE(BT_UUID_BAS_VAL),
			  BT_UUID_16_ENCODE(BT_UUID_CTS_VAL)),
};

static void bt_ready(void)
{
	int err;

	printk("Peripheral Bluetooth initialized\n");

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	if (err) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}

	printk("Advertising successfully started\n");
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

static bool volatile is_periodic;
static bt_addr_le_t per_addr;
static uint8_t per_sid;

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

static void test_broadcast_main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	printk("\n*PA Broadcaster*\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	setup_ext_adv(&adv);

	k_sleep(K_MSEC(40000));

	teardown_ext_adv(adv);
	adv = NULL;

	PASS("Broadcast PA Passed\n");
}

static void test_broadcast_past_sender_main(void)
{
	struct bt_le_ext_adv *adv;
	int err;

	printk("\n*Broadcaster*\n");

	printk("Connection callbacks register...\n");
	bt_conn_cb_register(&conn_callbacks);
	printk("Success.\n");

	printk("Bluetooth initializing...");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scanning for peripheral\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for connection...\n");
	err = k_sem_take(&sem_is_conn, K_SEM_TIMEOUT);
	if (err) {
		FAIL("Failed to connect (err %d)\n", err);
		return;
	}
	printk("success.\n");

	setup_ext_adv(&adv);

	k_sleep(K_MSEC(500));

	printk("Connection established and broadcasting - sending PAST\n");
	err = bt_le_per_adv_set_info_transfer(adv, default_conn, 0);
	if (err != 0) {
		FAIL("Could not transfer periodic adv sync: %d", err);
		return;
	}
	printk("success.\n");

	printk("Wait 20s for PAST to be send\n");
	k_sleep(K_SEM_TIMEOUT);

	printk("Disconnect before actually passing\n");
	err = bt_conn_disconnect(default_conn,
					BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Disconnection failed (err %d)\n", err);
		return;
	}
	printk("success.\n");

	teardown_ext_adv(adv);
	adv = NULL;

	PASS("Broadcast PA Passed\n");
}

static void test_past_send_main(void)
{
	struct bt_le_scan_param scan_param = {
		.type	   = BT_LE_SCAN_TYPE_ACTIVE,
		.options	= BT_LE_SCAN_OPT_NONE,
		.interval   = 0x0004,
		.window	 = 0x0004,
	};
	struct bt_le_per_adv_sync_param sync_create_param;
	struct bt_le_per_adv_sync *sync = NULL;
	uint16_t service_data = 0;
	int err;

	printk("\n*Send PAST test*\n");

	printk("Connection callbacks register...\n");
	bt_conn_cb_register(&conn_callbacks);
	printk("Success.\n");

	printk("Bluetooth initializing...\n");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register...\n");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...\n");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	printk("Start scanning...\n");
	is_periodic = false;
	err = bt_le_scan_start(&scan_param, NULL);
	if (err) {
		FAIL("Could not start scan: %d\n", err);
		return;
	}
	printk("success.\n");

	while (!is_periodic) {
		k_sleep(K_MSEC(100));
	}
	printk("Periodic Advertising found (SID: %u)\n", per_sid);

	printk("Creating Periodic Advertising Sync...\n");
	bt_addr_le_copy(&sync_create_param.addr, &per_addr);
	sync_create_param.options =
		BT_LE_PER_ADV_SYNC_OPT_REPORTING_INITIALLY_DISABLED;
	sync_create_param.sid = per_sid;
	sync_create_param.skip = 0;
	sync_create_param.timeout = 0xa;

	err = bt_le_per_adv_sync_create(&sync_create_param, &sync);
	if (err) {
		FAIL("Could not create sync: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for sync...\n");
	err = k_sem_take(&sem_is_sync, K_SEM_TIMEOUT);
	if (err) {
		FAIL("failed (err %d)\n", err);
		return;
	}
	printk("success.\n");


	printk("Scanning for peripheral\n");
	err = bt_le_scan_start(BT_LE_SCAN_ACTIVE, device_found);
	if (err) {
		FAIL("Scanning failed to start (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Waiting for connection...\n");
	err = k_sem_take(&sem_is_conn, K_SEM_TIMEOUT);
	if (err) {
		FAIL("Failed to connect (err %d)\n", err);
		return;
	}
	printk("success.\n");

	k_sleep(K_MSEC(1000));

	printk("Connection established - sending PAST\n");
	err = bt_le_per_adv_sync_transfer(sync, default_conn, service_data);
	if (err != 0) {
		FAIL("Could not transfer periodic adv sync: %d", err);
		return;
	}
	printk("success.\n");

	printk("Wait 20s for PAST to be send\n");
	k_sleep(K_SEM_TIMEOUT);

	printk("Disconnect before actually passing\n");
	err = bt_conn_disconnect(default_conn,
					BT_HCI_ERR_REMOTE_USER_TERM_CONN);
	if (err) {
		FAIL("Disconnection failed (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Deleting Periodic Advertising Sync...\n");
	err = bt_le_per_adv_sync_delete(sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	PASS("PAST send test Passed\n");
}

static void test_past_recv_main(void)
{
	struct bt_le_per_adv_sync_transfer_param past_param;
	int err;

	printk("\n*Receive PAST Test*\n");

	printk("Connection callbacks register...\n");
	bt_conn_cb_register(&conn_callbacks);
	printk("Success.\n");

	printk("Bluetooth initializing...\n");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register...\n");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...\n");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	printk("Set default PAST Params.\n");
	past_param.skip = 1;
	past_param.timeout = 1000; /* 10 seconds */
	past_param.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_FILTER_DUPLICATES;

	err = bt_le_per_adv_sync_transfer_subscribe(NULL, &past_param);
	if (err) {
		FAIL("Failed to set default past parameters(err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	bt_ready();

	printk("Waiting for connection...\n");
	err = k_sem_take(&sem_is_conn, K_SEM_TIMEOUT);
	if (err) {
		FAIL("Failed to connect (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Set PAST parameters for connection...\n");
	err = bt_le_per_adv_sync_transfer_subscribe(default_conn, &past_param);
	if (err) {
		FAIL("Failed to set default past parameters(err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	printk("Wait 20s for Periodic advertisement sync to be established\n");
	err = k_sem_take(&sem_is_sync, K_SEM_TIMEOUT);
	if (err) {
		FAIL("failed (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Deleting Periodic Advertising Sync...");
	err = bt_le_per_adv_sync_delete(default_sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	PASS("PAST recv test Passed\n");
}

static void test_past_recv_main_default_param(void)
{
	struct bt_le_per_adv_sync_transfer_param past_param;
	int err;

	printk("\n*Receive PAST Test*\n");

	printk("Connection callbacks register...\n");
	bt_conn_cb_register(&conn_callbacks);
	printk("Success.\n");

	printk("Bluetooth initializing...\n");
	err = bt_enable(NULL);
	if (err) {
		FAIL("Could not init BT: %d\n", err);
		return;
	}
	printk("success.\n");

	printk("Scan callbacks register...\n");
	bt_le_scan_cb_register(&scan_callbacks);
	printk("success.\n");

	printk("Periodic Advertising callbacks register...\n");
	bt_le_per_adv_sync_cb_register(&sync_cb);
	printk("Success.\n");

	printk("Set default PAST Params.\n");
	past_param.skip = 1;
	past_param.timeout = 1000; /* 10 seconds */
	past_param.options = BT_LE_PER_ADV_SYNC_TRANSFER_OPT_FILTER_DUPLICATES;

	err = bt_le_per_adv_sync_transfer_subscribe(NULL, &past_param);
	if (err) {
		FAIL("Failed to set default past parameters(err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	bt_ready();

	printk("Waiting for connection...\n");
	err = k_sem_take(&sem_is_conn, K_SEM_TIMEOUT);
	if (err) {
		FAIL("Failed to connect (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Wait 20s for Periodic advertisement sync to be established\n");
	err = k_sem_take(&sem_is_sync, K_SEM_TIMEOUT);
	if (err) {
		FAIL("failed (err %d)\n", err);
		return;
	}
	printk("success.\n");

	printk("Deleting Periodic Advertising Sync...");
	err = bt_le_per_adv_sync_delete(default_sync);
	if (err) {
		FAIL("Failed to delete periodic advertising sync (err %d)\n",
			 err);
		return;
	}
	printk("success.\n");

	PASS("PAST recv test Passed\n");
}

static void test_past_init(void)
{
	bst_ticker_set_next_tick_absolute(60e6);
	bst_result = In_progress;
}

static void test_past_tick(bs_time_t HW_device_time)
{
	if (bst_result != Passed) {
		FAIL("test failed (not passed after seconds)\n");
	}
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "broadcast_pa",
		.test_descr = "Periodic Advertisement broadcaster",
		.test_pre_init_f = test_past_init,
		.test_tick_f = test_past_tick,
		.test_main_f = test_broadcast_main
	},
	{
		.test_id = "receive_past",
		.test_descr = "Peripheral device, waiting for connection "
		"and then waits for receiving PAST, then syncs to PA",
		.test_pre_init_f = test_past_init,
		.test_tick_f = test_past_tick,
		.test_main_f = test_past_recv_main
	},
	{
		.test_id = "receive_past_default_param",
		.test_descr = "Peripheral device, waiting for connection "
		"and then waits for receiving PAST with the default PAST parameter set,"
		" then syncs to PA",
		.test_pre_init_f = test_past_init,
		.test_tick_f = test_past_tick,
		.test_main_f = test_past_recv_main_default_param
	},
	{
		.test_id = "send_past",
		.test_descr = "Central that syncs to PA from broadcaster,"
		"connects to peripheral and sends PAST",
		.test_pre_init_f = test_past_init,
		.test_tick_f = test_past_tick,
		.test_main_f = test_past_send_main
	},
	{
		.test_id = "broadcast_past_sender",
		.test_descr = "PA broadcaster, connects and sends PAST to peripheral",
		.test_pre_init_f = test_past_init,
		.test_tick_f = test_past_tick,
		.test_main_f = test_broadcast_past_sender_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_past_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
