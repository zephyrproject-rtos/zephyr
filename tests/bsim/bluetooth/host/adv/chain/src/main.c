/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stddef.h>

#include <zephyr/kernel.h>

#include <zephyr/sys/printk.h>
#include <zephyr/sys/util.h>

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "babblekit/testcase.h"

#define NAME_LEN 30
#define AD_DATA_NAME_SIZE     (sizeof(CONFIG_BT_DEVICE_NAME) - 1U + 2U)
#define AD_DATA_MFG_DATA_SIZE (254U + 2U)
/*
 * for testing chaining the manufacturing data is duplicated, hence DATA_LEN needs to
 * add twice the size for this element
 */
#define DATA_LEN                 MIN((AD_DATA_NAME_SIZE + \
				      AD_DATA_MFG_DATA_SIZE + AD_DATA_MFG_DATA_SIZE), \
				     CONFIG_BT_CTLR_ADV_DATA_LEN_MAX)

/* One less extended advertising set as first one is legacy advertising in the broadcaster_multiple
 * sample.
 */
#define MAX_EXT_ADV_SET (CONFIG_BT_EXT_ADV_MAX_ADV_SET - 1)

static K_SEM_DEFINE(sem_recv, 0, 1);

static void test_adv_main(void)
{
	extern int broadcaster_multiple(void);
	int err;

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed");

		bs_trace_silent_exit(err);
		return;
	}

	err = broadcaster_multiple();
	if (err) {
		TEST_FAIL("Adv tests failed");
		bs_trace_silent_exit(err);
		return;
	}

	/* Successfully started advertising multiple sets */
	TEST_PASS("Adv tests passed");

	/* Let the scanner receive the reports */
	k_sleep(K_SECONDS(10));
}

static bool data_cb(struct bt_data *data, void *user_data)
{
	char *name = user_data;
	uint8_t len;

	switch (data->type) {
	case BT_DATA_NAME_SHORTENED:
	case BT_DATA_NAME_COMPLETE:
		len = MIN(data->data_len, NAME_LEN - 1);
		(void)memcpy(name, data->data, len);
		name[len] = '\0';
		return false;
	default:
		return true;
	}
}

static void scan_recv(const struct bt_le_scan_recv_info *info,
		      struct net_buf_simple *buf)
{
	static uint8_t sid[MAX_EXT_ADV_SET];
	static uint8_t sid_count;
	char name[NAME_LEN];
	uint8_t data_status;
	uint16_t data_len;

	data_status = BT_HCI_LE_ADV_EVT_TYPE_DATA_STATUS(info->adv_props);
	if (data_status) {
		printk("Received data status: %u\n", data_status);
		return;
	}

	data_len = buf->len;
	if (data_len != DATA_LEN) {
		printk("Received data length: %u\n", data_len);
		return;
	}

	(void)memset(name, 0, sizeof(name));
	bt_data_parse(buf, data_cb, name);

	if (strcmp(name, CONFIG_BT_DEVICE_NAME)) {
		printk("Wrong name %s\n", name);
		return;
	}

	for (uint8_t i = 0; i < sid_count; i++) {
		if (sid[i] == info->sid) {
			printk("Received SID %d\n", info->sid);
			return;
		}
	}

	sid[sid_count++] = info->sid;

	printk("Received advertising sets: %d\n", sid_count);

	if (sid_count < MAX_EXT_ADV_SET) {
		return;
	}

	k_sem_give(&sem_recv);
}

static struct bt_le_scan_cb scan_callbacks = {
	.recv = scan_recv,
};

static void test_scan_main(void)
{
	extern int observer_start(void);
	int err;

	err = bt_enable(NULL);
	if (err) {
		TEST_FAIL("Bluetooth init failed");

		bs_trace_silent_exit(err);
		return;
	}

	bt_le_scan_cb_register(&scan_callbacks);

	err = observer_start();
	if (err) {
		TEST_FAIL("Observer start failed");

		bs_trace_silent_exit(err);
		return;
	}

	/* Let the recv callback verify the reports; increase the wait to receive one each of
	 * legacy, Extended 2M, 1M and Coded PHY advertising reports. Depending on random seed
	 * and scan window, it may take some time to have all advertising report received.
	 */
	k_sleep(K_SECONDS(30));

	err = k_sem_take(&sem_recv, K_NO_WAIT);
	if (err) {
		TEST_FAIL("Scan receive failed");

		bs_trace_silent_exit(err);
		return;
	}

	TEST_PASS("Scan tests passed");

	bs_trace_silent_exit(0);
}

static const struct bst_test_instance test_def[] = {
	{
		.test_id = "adv",
		.test_descr = "Central GATT Write",
		.test_main_f = test_adv_main
	},
	{
		.test_id = "scan",
		.test_descr = "Peripheral GATT Write",
		.test_main_f = test_scan_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_chain_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_def);
}

bst_test_install_t test_installers[] = {
	test_adv_chain_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
