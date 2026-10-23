/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/gap.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/net_buf.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

#include "bstests.h"
#include "babblekit/testcase.h"

/* The static random identity address of the scanner, C0:00:00:00:00:02. The
 * directed advertising that test_data/gen_anonymous.py writes for
 * anonymous.sh has it as the TargetA.
 */
static const bt_addr_le_t scanner_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x02, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* A device that is not in the simulation, C0:00:00:00:00:03. The directed
 * advertising that test_data/gen_anonymous.py writes for anonymous_other.sh
 * has it as the TargetA.
 */
static const bt_addr_le_t other_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x03, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* The AdvA of the directed advertising to a resolvable private address,
 * C0:00:00:00:00:05, which the playback device of anonymous.sh sends with the
 * AdvA only in the AUX_ADV_IND
 */
static const bt_addr_le_t rpa_directed_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x05, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* The local IRK of the resolving list entry of the scanner, the IRK of the
 * sample data of the random address hash function ah (Core Spec v6.3, Vol 3,
 * Part H, Appendix D.7) in HCI octet order. The TargetA of the directed
 * advertising to a resolvable private address is generated from it.
 */
static const uint8_t local_irk[16] = {
	0x9b, 0x7d, 0x39, 0x0a, 0xa6, 0x10, 0x10, 0x34,
	0x05, 0xad, 0xc8, 0x57, 0xa3, 0x34, 0x02, 0xec,
};

/* The peer of the resolving list entry, a device that is not in the
 * simulation, C0:00:00:00:00:06, so that no AdvA selects the entry
 */
static const bt_addr_le_t rl_peer_addr = {
	.type = BT_ADDR_LE_RANDOM,
	.a = { { 0x06, 0x00, 0x00, 0x00, 0x00, 0xC0 } },
};

/* The AD data of the advertising in AUX_ADV_INDs, which the playback device
 * of the test scripts sends: anonymous undirected, directed with the TargetA
 * in the AUX_ADV_IND, and directed with it in the ADV_EXT_IND, and directed
 * with a resolvable private address in the ADV_EXT_IND that resolves with the
 * local IRK and one that does not
 */
#define AD_UNDIRECTED         0x01U
#define AD_DIRECTED           0x02U
#define AD_DIRECTED_PRI       0x03U
#define AD_DIRECTED_RPA       0x04U
#define AD_DIRECTED_RPA_OTHER 0x05U

/* The advertising set of the playback device whose AUX_ADV_IND announces a
 * TxPower that its extended header has no room for, which may not be reported
 */
#define SID_TRUNCATED 6U

/* The kinds of anonymous advertising reported. The advertising sets of the
 * advertiser device have no ADI and no AD data, so the reports do not tell
 * them apart otherwise.
 */
#define REPORT_UNDIRECTED      BIT(0)
#define REPORT_DIRECTED        BIT(1)
#define REPORT_UNDIRECTED_AD   BIT(2)
#define REPORT_DIRECTED_AD     BIT(3)
#define REPORT_DIRECTED_PRI_AD BIT(4)
#define REPORT_OTHER           BIT(5)
#define REPORT_TRUNCATED       BIT(6)
#define REPORT_DIRECTED_RPA    BIT(7)
#define REPORT_RPA_OTHER       BIT(8)

static atomic_t reports;

static void adv_start(const bt_addr_le_t *peer)
{
	struct bt_le_adv_param param = BT_LE_ADV_PARAM_INIT(
		BT_LE_ADV_OPT_EXT_ADV | BT_LE_ADV_OPT_ANONYMOUS, BT_GAP_ADV_FAST_INT_MIN_2,
		BT_GAP_ADV_FAST_INT_MAX_2, peer);
	struct bt_le_ext_adv *adv;
	int err;

	if (peer != NULL) {
		param.options |= BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY;
	}

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	TEST_ASSERT(err == 0, "Advertising set create failed (err %d)", err);

	err = bt_le_ext_adv_start(adv, BT_LE_EXT_ADV_START_DEFAULT);
	TEST_ASSERT(err == 0, "Advertising start failed (err %d)", err);
}

/* Undirected anonymous advertising, and directed to the given device. The
 * Controller sends no AuxPtr without AD data.
 */
static void anon_adv(const bt_addr_le_t *peer)
{
	int err;

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	adv_start(NULL);
	adv_start(peer);
}

static void test_anon_adv(void)
{
	TEST_START("anon_adv");

	anon_adv(&scanner_addr);

	/* The scanner checks which of the sets it reports */
	TEST_PASS("anon_adv");
}

static void test_anon_adv_other(void)
{
	TEST_START("anon_adv_other");

	anon_adv(&other_addr);

	/* The scanner checks which of the sets it reports */
	TEST_PASS("anon_adv_other");
}

static bool ad_parse(struct bt_data *data, void *user_data)
{
	uint8_t *ad_value = user_data;

	if ((data->type == BT_DATA_MANUFACTURER_DATA) && (data->data_len == 1U)) {
		*ad_value = data->data[0];

		return false;
	}

	return true;
}

static void scan_recv(const struct bt_le_scan_recv_info *info, struct net_buf_simple *buf)
{
	bool is_directed = ((info->adv_props & BT_GAP_ADV_PROP_DIRECTED) != 0U);
	uint8_t ad_value = 0U;

	if (bt_addr_le_eq(info->addr, &rpa_directed_addr)) {
		bt_data_parse(buf, ad_parse, &ad_value);

		(void)atomic_or(&reports, (is_directed && (ad_value == AD_DIRECTED_RPA)) ?
						  REPORT_DIRECTED_RPA :
						  REPORT_RPA_OTHER);

		return;
	}

	/* The Host reports anonymous advertising as from BT_ADDR_LE_ANY */
	if (!bt_addr_le_eq(info->addr, BT_ADDR_LE_ANY)) {
		return;
	}

	if (info->sid == SID_TRUNCATED) {
		(void)atomic_or(&reports, REPORT_TRUNCATED);

		return;
	}

	bt_data_parse(buf, ad_parse, &ad_value);

	if (ad_value == 0U) {
		(void)atomic_or(&reports, is_directed ? REPORT_DIRECTED : REPORT_UNDIRECTED);
	} else if (!is_directed && (ad_value == AD_UNDIRECTED)) {
		(void)atomic_or(&reports, REPORT_UNDIRECTED_AD);
	} else if (is_directed && (ad_value == AD_DIRECTED)) {
		(void)atomic_or(&reports, REPORT_DIRECTED_AD);
	} else if (is_directed && (ad_value == AD_DIRECTED_PRI)) {
		(void)atomic_or(&reports, REPORT_DIRECTED_PRI_AD);
	} else {
		(void)atomic_or(&reports, REPORT_OTHER);
	}
}

static struct bt_le_scan_cb scan_cb = {
	.recv = scan_recv,
};

static atomic_val_t scan(uint8_t options)
{
	/* Active, as anonymous.sh checks which advertising gets AUX_SCAN_REQs */
	struct bt_le_scan_param param = BT_LE_SCAN_PARAM_INIT(
		BT_LE_SCAN_TYPE_ACTIVE, options, BT_GAP_SCAN_FAST_INTERVAL,
		BT_GAP_SCAN_FAST_INTERVAL);
	int err;

	(void)atomic_clear(&reports);

	err = bt_le_scan_start(&param, NULL);
	TEST_ASSERT(err == 0, "Scan start failed (err %d)", err);

	/* Scan for a fixed time, as the kinds that are not reported are part
	 * of what is checked. The playback device of the test scripts sends
	 * for the first 6 seconds of the simulation, which two scans fit in.
	 */
	(void)k_sleep(K_SECONDS(3));

	err = bt_le_scan_stop();
	TEST_ASSERT(err == 0, "Scan stop failed (err %d)", err);

	return atomic_get(&reports);
}

/* A resolving list entry with the local IRK, and address resolution enabled,
 * through HCI, as the Host adds entries only for bonded peers
 */
static void local_irk_add(void)
{
	struct bt_hci_cp_le_set_addr_res_enable *enable;
	struct bt_hci_cp_le_add_dev_to_rl *entry;
	struct net_buf *buf;
	int err;

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No HCI command buffer");

	entry = net_buf_add(buf, sizeof(*entry));
	bt_addr_le_copy(&entry->peer_id_addr, &rl_peer_addr);
	(void)memset(entry->peer_irk, 0, sizeof(entry->peer_irk));
	(void)memcpy(entry->local_irk, local_irk, sizeof(entry->local_irk));

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_ADD_DEV_TO_RL, buf, NULL);
	TEST_ASSERT(err == 0, "Resolving list add failed (err %d)", err);

	buf = bt_hci_cmd_alloc(K_FOREVER);
	TEST_ASSERT(buf != NULL, "No HCI command buffer");

	enable = net_buf_add(buf, sizeof(*enable));
	enable->enable = BT_HCI_ADDR_RES_ENABLE;

	err = bt_hci_cmd_send_sync(BT_HCI_OP_LE_SET_ADDR_RES_ENABLE, buf, NULL);
	TEST_ASSERT(err == 0, "Address resolution enable failed (err %d)", err);
}

static void scanner_init(void)
{
	bt_addr_le_t addr = scanner_addr;
	int err;

	err = bt_id_create(&addr, NULL);
	TEST_ASSERT(err >= 0, "Identity create failed (err %d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)", err);

	local_irk_add();

	err = bt_le_scan_cb_register(&scan_cb);
	TEST_ASSERT(err == 0, "Scan callback register failed (err %d)", err);
}

static void test_anon_scan(void)
{
	atomic_val_t reported;

	TEST_START("anon_scan");

	scanner_init();

	reported = scan(BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(reported == (REPORT_UNDIRECTED | REPORT_DIRECTED | REPORT_UNDIRECTED_AD |
				 REPORT_DIRECTED_AD | REPORT_DIRECTED_PRI_AD | REPORT_DIRECTED_RPA),
		    "Anonymous advertising reported as 0x%lx", (unsigned long)reported);

	/* The Filter Accept List has no entry for anonymous advertising, nor
	 * for the advertiser of the directed advertising to a resolvable
	 * private address
	 */
	reported = scan(BT_LE_SCAN_OPT_FILTER_ACCEPT_LIST);
	TEST_ASSERT(reported == 0, "Anonymous advertising reported as 0x%lx with the list",
		    (unsigned long)reported);

	TEST_PASS("anon_scan");
}

static void test_anon_scan_other(void)
{
	atomic_val_t reported;

	TEST_START("anon_scan_other");

	scanner_init();

	reported = scan(BT_LE_SCAN_OPT_NONE);
	TEST_ASSERT(reported == (REPORT_UNDIRECTED | REPORT_UNDIRECTED_AD),
		    "Anonymous advertising reported as 0x%lx", (unsigned long)reported);

	TEST_PASS("anon_scan_other");
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "anon_adv",
		.test_descr = "Anonymous advertising, undirected and directed to the scanner",
		.test_main_f = test_anon_adv,
	},
	{
		.test_id = "anon_adv_other",
		.test_descr = "Anonymous advertising, undirected and directed to another device",
		.test_main_f = test_anon_adv_other,
	},
	{
		.test_id = "anon_scan",
		.test_descr = "Passive scanner of anonymous advertising to the scanner",
		.test_main_f = test_anon_scan,
	},
	{
		.test_id = "anon_scan_other",
		.test_descr = "Passive scanner of anonymous advertising to another device",
		.test_main_f = test_anon_scan_other,
	},
	BSTEST_END_MARKER,
};

struct bst_test_list *test_anonymous_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}
