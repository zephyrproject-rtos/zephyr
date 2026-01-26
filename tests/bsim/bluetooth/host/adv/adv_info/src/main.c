/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

#if defined(CONFIG_BT_PRIVACY)
#define RPA_TIMEOUT (CONFIG_BT_RPA_TIMEOUT)
#else
#define RPA_TIMEOUT (0)
#endif

static const struct bt_data ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, BT_LE_AD_NO_BREDR),
};

static void test_adv_info(void)
{
	struct bt_le_adv_info info = {};
	bt_addr_le_t identity_addrs[CONFIG_BT_ID_MAX] = {};
	bt_addr_le_t rpa_addr = {};
	size_t identity_count = ARRAY_SIZE(identity_addrs);
	int err;

	/* Initialize the Bluetooth Subsystem */
	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "Bluetooth init failed (err %d)\n", err);

	printk("Bluetooth initialized\n");

	err = bt_le_adv_get_info(&info);
	TEST_ASSERT(err == -EINVAL, "Get adv info failed incorrectly (err %d)\n", err);

	/* Start advertising */
	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Advertising failed to start (err %d)\n", err);

	printk("Advertising started\n");

	err = bt_le_adv_get_info(&info);
	TEST_ASSERT(err == 0, "Get adv info failed (err %d)\n", err);

	printk("Advertising identity handle: %u\n", info.id);

	char addr_str[BT_ADDR_LE_STR_LEN];

	(void)bt_addr_le_to_str(info.addr, addr_str, sizeof(addr_str));
	printk("Advertising address: %s\n", addr_str);

	TEST_ASSERT(info.id == BT_ID_DEFAULT, "Advertising with wrong identity handle\n");
	TEST_ASSERT(info.addr->type == BT_ADDR_LE_RANDOM,
		    "Advertising with wrong address type: %d\n", info.addr->type);
	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		TEST_ASSERT(BT_ADDR_IS_RPA(&info.addr->a),
			    "Advertising address is not RPA\n");

		bt_addr_le_copy(&rpa_addr, info.addr);

		k_sleep(K_SECONDS(RPA_TIMEOUT + 1));

		err = bt_le_adv_get_info(&info);
		TEST_ASSERT(err == 0, "Get adv info after RPA timeout failed (err %d)\n", err);

		char addr_str2[BT_ADDR_LE_STR_LEN];
		(void)bt_addr_le_to_str(info.addr, addr_str2, sizeof(addr_str2));
		printk("Advertising address after RPA timeout: %s\n", addr_str2);

		TEST_ASSERT(!bt_addr_le_eq(&rpa_addr, info.addr),
			    "Advertising address did not rotate\n");
	} else {
		TEST_ASSERT(BT_ADDR_IS_STATIC(&info.addr->a),
			    "Advertising address is not static random\n");
	}

	err = bt_le_adv_stop();
	TEST_ASSERT(err == 0, "Advertising failed to stop (err %d)\n", err);

	err = bt_le_adv_get_info(&info);
	TEST_ASSERT(err == -EINVAL, "Get adv info failed incorrectly (err %d)\n", err);

	bt_id_get(identity_addrs, &identity_count);
	TEST_ASSERT(identity_count > BT_ID_DEFAULT,
		    "Failed to fetch identity address\n");

	err = bt_le_adv_start(BT_LE_ADV_NCONN_IDENTITY, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "Identity advertising failed to start (err %d)\n", err);

	err = bt_le_adv_get_info(&info);
	TEST_ASSERT(err == 0, "Get identity adv info failed (err %d)\n", err);

	TEST_ASSERT(bt_addr_le_eq(info.addr, &identity_addrs[BT_ID_DEFAULT]),
		    "Advertising address does not match identity address\n");

	TEST_PASS("Test passed");
}

static const struct bst_test_instance adv_info_tc[] = {
	{
		.test_id = "adv_info",
		.test_descr = "Basic advertising info retrieval test",
		.test_main_f = test_adv_info,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_adv_info_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, adv_info_tc);
}

bst_test_install_t test_installers[] = {
	test_adv_info_install,
	NULL
};

int main(void)
{
	bst_main();
	return 0;
}
