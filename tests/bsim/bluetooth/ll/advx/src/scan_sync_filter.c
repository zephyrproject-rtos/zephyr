/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 * SPDX-License-Identifier: Apache-2.0
 */

/* While a periodic advertising sync is being created, the scanner must still not report the legacy
 * advertising PDUs that its filter policy or address resolution rejects, nor send a scan request to
 * an advertiser that its filter policy rejects.
 */

#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>

#include "bstests.h"
#include "babblekit/sync.h"
#include "babblekit/testcase.h"

/* The static random identity address of the legacy advertiser */
static const bt_addr_le_t adv_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {{0x03, 0x00, 0x00, 0x00, 0x00, 0xC0}},
};

/* The Advertising SID of the periodic advertiser */
#define PER_ADV_SID 0x05U

static atomic_t adv_reports;

static void scan_recv(const bt_addr_le_t *addr, int8_t rssi, uint8_t adv_type,
		      struct net_buf_simple *ad)
{
	if (bt_addr_le_eq(addr, &adv_addr)) {
		(void)atomic_inc(&adv_reports);
	}
}

/* Scan for two seconds and return the number of reports of the advertiser */
static atomic_val_t scan_count(uint8_t type, uint32_t options)
{
	int err;

	/* Report every advertising PDU, so that no report is held back as a duplicate */
	err = bt_le_scan_start(
		BT_LE_SCAN_PARAM(type, options, BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW),
		scan_recv);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	/* Count only from now on, with these scan parameters in use */
	atomic_clear(&adv_reports);

	(void)k_sleep(K_SECONDS(2));

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Scan stop failed (err %d)", err);

	return atomic_get(&adv_reports);
}

/* A periodic advertising sync to a device that is not there, which stays pending */
static void per_adv_sync_create(void)
{
	/* The address of a device that is not there */
	static const bt_addr_le_t absent_pa_addr = {
		.type = BT_ADDR_LE_RANDOM,
		.a = {{0x05, 0x00, 0x00, 0x00, 0x00, 0xC0}},
	};
	struct bt_le_per_adv_sync_param param = {0};
	struct bt_le_per_adv_sync *sync;
	int err;

	bt_addr_le_copy(&param.addr, &absent_pa_addr);
	param.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;

	err = bt_le_per_adv_sync_create(&param, &sync);
	TEST_ASSERT(err == 0, "Periodic advertising sync create failed (err %d)", err);
}

static void hci_cmd(uint16_t opcode, struct net_buf *buf)
{
	int err;

	err = bt_hci_cmd_send_sync(opcode, buf, NULL);
	TEST_ASSERT(err == 0, "HCI command 0x%04x failed (err %d)", opcode, err);
}

static void scanner_init(void)
{
	atomic_val_t count;
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	/* The advertiser is heard while nothing filters it */
	count = scan_count(BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(count > 0, "No report of the advertiser");
}

/* The address of a device that is not there, as the only Filter Accept List entry */
static const bt_addr_le_t other_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = {{0x04, 0x00, 0x00, 0x00, 0x00, 0xC0}},
};

static void test_scan_sync_fal(void)
{
	atomic_val_t count;
	int err;

	TEST_START("scan_sync_fal");

	scanner_init();

	err = bt_le_filter_accept_list_add(&other_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	per_adv_sync_create();

	count = scan_count(BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST);
	TEST_ASSERT(count == 0, "%ld reports of the advertiser, which is not on the list", count);

	TEST_PASS("scan_sync_fal");
}

static void test_scan_sync_rl(void)
{
	struct bt_hci_cp_le_add_dev_to_rl *cp;
	struct net_buf *buf;
	atomic_val_t count;

	TEST_START("scan_sync_rl");

	scanner_init();

	/* The advertiser in the resolving list with an IRK, in the default Network Privacy mode */
	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "HCI command alloc failed");
	cp = net_buf_add(buf, sizeof(*cp));
	bt_addr_le_copy(&cp->peer_id_addr, &adv_addr);
	(void)memset(cp->peer_irk, 0x11, sizeof(cp->peer_irk));
	(void)memset(cp->local_irk, 0, sizeof(cp->local_irk));
	hci_cmd(BT_HCI_OP_LE_ADD_DEV_TO_RL, buf);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "HCI command alloc failed");
	net_buf_add_u8(buf, BT_HCI_ADDR_RES_ENABLE);
	hci_cmd(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE, buf);

	per_adv_sync_create();

	count = scan_count(BT_LE_SCAN_TYPE_PASSIVE, BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(count == 0, "%ld reports of the identity address of a Network Privacy peer",
		    count);

	TEST_PASS("scan_sync_rl");
}

static void test_scan_sync_active(void)
{
	atomic_val_t count;
	int err;

	TEST_START("scan_sync_active");

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed (err %d)", err);

	scanner_init();

	/* Let the advertiser check that it gets scan requests from an active scanner, and wait for
	 * it to start counting again
	 */
	(void)scan_count(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_NONE);
	bk_sync_send();
	bk_sync_wait();

	err = bt_le_filter_accept_list_add(&other_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	per_adv_sync_create();

	count = scan_count(BT_LE_SCAN_TYPE_ACTIVE, BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST);
	TEST_ASSERT(count == 0, "%ld reports of the advertiser, which is not on the list", count);

	/* Let the advertiser check that it got no scan request since the first sync */
	bk_sync_send();

	TEST_PASS("scan_sync_active");
}

static K_SEM_DEFINE(synced_sem, 0, 1);

static void synced(struct bt_le_per_adv_sync *sync, struct bt_le_per_adv_sync_synced_info *info)
{
	k_sem_give(&synced_sem);
}

static struct bt_le_per_adv_sync_cb sync_cb = {
	.synced = synced,
};

/* The scan filters reject the periodic advertiser, which the sync must still find */
static void scan_sync_found(bool use_list)
{
	struct bt_le_per_adv_sync_param param = {0};
	struct bt_le_per_adv_sync *sync;
	int err;

	scanner_init();

	bt_le_per_adv_sync_cb_register(&sync_cb);

	err = bt_le_filter_accept_list_add(&other_addr);
	TEST_ASSERT(err == 0, "Filter Accept List add failed (err %d)", err);

	if (use_list) {
		err = bt_le_per_adv_list_add(&adv_addr, PER_ADV_SID);
		TEST_ASSERT(err == 0, "Periodic Advertiser List add failed (err %d)", err);
		param.options = BT_LE_PER_ADV_SYNC_OPT_USE_PER_ADV_LIST;
	} else {
		bt_addr_le_copy(&param.addr, &adv_addr);
		param.sid = PER_ADV_SID;
	}
	param.timeout = BT_GAP_PER_ADV_MAX_TIMEOUT;

	err = bt_le_per_adv_sync_create(&param, &sync);
	TEST_ASSERT(err == 0, "Periodic advertising sync create failed (err %d)", err);

	err = bt_le_scan_start(BT_LE_SCAN_PARAM(BT_LE_SCAN_TYPE_PASSIVE,
						BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST,
						BT_GAP_SCAN_FAST_INTERVAL, BT_GAP_SCAN_FAST_WINDOW),
			       NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	err = k_sem_take(&synced_sem, K_SECONDS(5));
	TEST_ASSERT(err == 0, "No sync to the periodic advertiser, which is not on the list");
}

static void test_scan_sync_found_addr(void)
{
	TEST_START("scan_sync_found_addr");

	scan_sync_found(false);

	TEST_PASS("scan_sync_found_addr");
}

static void test_scan_sync_found_list(void)
{
	TEST_START("scan_sync_found_list");

	scan_sync_found(true);

	TEST_PASS("scan_sync_found_list");
}

static atomic_t scan_reqs;

static void adv_scanned(struct bt_le_ext_adv *adv, struct bt_le_ext_adv_scanned_info *info)
{
	(void)atomic_inc(&scan_reqs);
}

static const struct bt_le_ext_adv_cb adv_cb = {
	.scanned = adv_scanned,
};

/* Scannable advertising from the identity address, counting the scan requests */
static void scannable_adv(uint32_t options)
{
	const struct bt_data sd[] = {
		BT_DATA_BYTES(BT_DATA_NAME_COMPLETE, 'A', 'd', 'v', 'X'),
	};
	bt_addr_le_t addr = adv_addr;
	struct bt_le_ext_adv *adv;
	int err;

	err = bk_sync_init();
	TEST_ASSERT(err == 0, "Sync init failed (err %d)", err);

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	err = bt_le_ext_adv_create(
		BT_LE_ADV_PARAM(BT_LE_ADV_OPT_SCANNABLE | BT_LE_ADV_OPT_NOTIFY_SCAN_REQ |
					BT_LE_ADV_OPT_USE_IDENTITY | options,
				BT_GAP_ADV_FAST_INT_MIN_2, BT_GAP_ADV_FAST_INT_MAX_2, NULL),
		&adv_cb, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_set_data(adv, NULL, 0, sd, ARRAY_SIZE(sd));
	TEST_ASSERT(err == 0, "Scan response data set failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	bk_sync_wait();
	TEST_ASSERT(atomic_get(&scan_reqs) > 0, "No scan request from an active scanner");
	atomic_clear(&scan_reqs);
	bk_sync_send();

	/* The scanner is done, and the notification of a scan request is not delayed further */
	bk_sync_wait();
	(void)k_sleep(K_MSEC(100));
	TEST_ASSERT(atomic_get(&scan_reqs) == 0,
		    "%ld scan requests from a scanner that does not have us on its list",
		    atomic_get(&scan_reqs));
}

static void test_legacy_adv_scannable(void)
{
	TEST_START("legacy_adv_scannable");

	scannable_adv(0U);

	TEST_PASS("legacy_adv_scannable");
}

static void test_ext_adv_scannable(void)
{
	TEST_START("ext_adv_scannable");

	scannable_adv(BT_LE_ADV_OPT_EXT_ADV);

	TEST_PASS("ext_adv_scannable");
}

static void test_per_adv(void)
{
	struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_USE_IDENTITY, BT_GAP_ADV_FAST_INT_MIN_2,
		BT_GAP_ADV_FAST_INT_MAX_2, NULL);
	bt_addr_le_t addr = adv_addr;
	struct bt_le_ext_adv *adv;
	int err;

	TEST_START("per_adv");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	param.sid = PER_ADV_SID;
	err = bt_le_ext_adv_create(&param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_per_adv_set_param(adv, BT_LE_PER_ADV_DEFAULT);
	TEST_ASSERT(err == 0, "Periodic advertising parameters failed (err %d)", err);

	err = bt_le_per_adv_start(adv);
	TEST_ASSERT(err == 0, "Periodic advertising start failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	TEST_PASS("per_adv");
}

static void test_legacy_adv(void)
{
	bt_addr_le_t addr = adv_addr;
	int err;

	TEST_START("legacy_adv");

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	/* Legacy advertising from the identity address */
	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, NULL, 0, NULL, 0);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);

	TEST_PASS("legacy_adv");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "scan_sync_fal",
		.test_descr = "Scanner using the Filter Accept List, with a periodic sync being "
			      "created, which must not report a legacy advertiser not on the list",
		.test_main_f = test_scan_sync_fal,
	},
	{
		.test_id = "scan_sync_rl",
		.test_descr = "Scanner with a Network Privacy peer in the resolving list, with a "
			      "periodic sync being created, which must not report the peer's "
			      "identity address",
		.test_main_f = test_scan_sync_rl,
	},
	{
		.test_id = "scan_sync_found_addr",
		.test_descr = "Scanner using the Filter Accept List, which syncs to a periodic "
			      "advertiser given by its address, which is not on the list",
		.test_main_f = test_scan_sync_found_addr,
	},
	{
		.test_id = "scan_sync_found_list",
		.test_descr = "Scanner using the Filter Accept List, which syncs to a periodic "
			      "advertiser on its Periodic Advertiser List, which is not on the list",
		.test_main_f = test_scan_sync_found_list,
	},
	{
		.test_id = "scan_sync_active",
		.test_descr = "Active scanner using the Filter Accept List, with a periodic sync "
			      "being created, which must not send a scan request to an advertiser "
			      "not on the list",
		.test_main_f = test_scan_sync_active,
	},
	{
		.test_id = "legacy_adv_scannable",
		.test_descr = "Scannable legacy advertiser counting the scan requests",
		.test_main_f = test_legacy_adv_scannable,
	},
	{
		.test_id = "ext_adv_scannable",
		.test_descr = "Scannable extended advertiser counting the scan requests",
		.test_main_f = test_ext_adv_scannable,
	},
	{
		.test_id = "per_adv",
		.test_descr = "Periodic advertiser using its identity address",
		.test_main_f = test_per_adv,
	},
	{
		.test_id = "legacy_adv",
		.test_descr = "Non-connectable legacy advertiser using its identity address",
		.test_main_f = test_legacy_adv,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_scan_sync_filter_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
