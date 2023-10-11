/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_CAP_ACCEPTOR)

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/audio/cap.h>
#include "common.h"
#include "unicast_common.h"

extern enum bst_result_t bst_result;

/* TODO: Expand with CAP service data */
static const struct bt_data cap_acceptor_ad[] = {
	BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
	BT_DATA_BYTES(BT_DATA_UUID16_ALL, BT_UUID_16_ENCODE(BT_UUID_CAS_VAL)),
};

static struct bt_csip_set_member_svc_inst *csip_set_member;

CREATE_FLAG(flag_connected);

static void connected(struct bt_conn *conn, uint8_t err)
{
	char addr[BT_ADDR_LE_STR_LEN];

	bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

	if (err != 0) {
		FAIL("Failed to connect to %s (%u)\n", addr, err);
		return;
	}

	printk("Connected to %s\n", addr);
	SET_FLAG(flag_connected);
}

BT_CONN_CB_DEFINE(conn_callbacks) = {
	.connected = connected,
	.disconnected = disconnected,
};

static void init(void)
{
	struct bt_csip_set_member_register_param csip_set_member_param = {
		.set_size = 3,
		.rank = 1,
		.lockable = true,
		/* Using the CSIP_SET_MEMBER test sample SIRK */
		.set_sirk = { 0xcd, 0xcc, 0x72, 0xdd, 0x86, 0x8c, 0xcd, 0xce,
			0x22, 0xfd, 0xa1, 0x21, 0x09, 0x7d, 0x7d, 0x45 },
	};
	int err;

	err = bt_enable(NULL);
	if (err != 0) {
		FAIL("Bluetooth enable failed (err %d)\n", err);
		return;
	}

	printk("Bluetooth initialized\n");

	if (IS_ENABLED(CONFIG_BT_CAP_ACCEPTOR_SET_MEMBER)) {
		err = bt_cap_acceptor_register(&csip_set_member_param,
					       &csip_set_member);
		if (err != 0) {
			FAIL("CAP acceptor failed to register (err %d)\n", err);
			return;
		}
	}

	err = bt_le_adv_start(BT_LE_ADV_CONN_NAME, cap_acceptor_ad,
			      ARRAY_SIZE(cap_acceptor_ad), NULL, 0);
	if (err != 0) {
		FAIL("Advertising failed to start (err %d)\n", err);
		return;
	}
}

static void test_main(void)
{
	init();

	/* TODO: When babblesim supports ISO, wait for audio stream to pass */

	WAIT_FOR_FLAG(flag_connected);

	PASS("CAP acceptor passed\n");
}

static const struct bst_test_instance test_cap_acceptor[] = {
	{
		.test_id = "cap_acceptor",
		.test_post_init_f = test_init,
		.test_tick_f = test_tick,
		.test_main_f = test_main
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_cap_acceptor_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_cap_acceptor);
}

#else /* !(CONFIG_BT_CAP_ACCEPTOR) */

struct bst_test_list *test_cap_acceptor_install(struct bst_test_list *tests)
{
	return tests;
}

#endif /* CONFIG_BT_CAP_ACCEPTOR */
