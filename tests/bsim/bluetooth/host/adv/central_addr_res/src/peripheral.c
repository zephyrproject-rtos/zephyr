/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>

#include <zephyr/kernel.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gap.h>

#include "babblekit/testcase.h"
#include "babblekit/flags.h"

DEFINE_FLAG_STATIC(flag_connected);
DEFINE_FLAG_STATIC(flag_bonded);
DEFINE_FLAG_STATIC(flag_support_read);

static enum bt_le_addr_res_support read_support;

static struct bt_conn *g_conn;

static void connected(struct bt_conn *conn, uint8_t err)
{
	if (err != 0) {
		TEST_FAIL("Failed to connect to %s (%u)", bt_conn_dst_str(conn), err);
		return;
	}

	g_conn = bt_conn_ref(conn);
	SET_FLAG(flag_connected);
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
	if (conn != g_conn) {
		return;
	}

	bt_conn_drop(&g_conn);
	UNSET_FLAG(flag_connected);
}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
	if (!bonded) {
		TEST_FAIL("Pairing did not create a bond");
		return;
	}

	SET_FLAG(flag_bonded);
}

static void addr_res_support_read(struct bt_conn *conn, enum bt_le_addr_res_support support)
{
	read_support = support;
	SET_FLAG(flag_support_read);
}

static struct bt_conn_auth_info_cb auth_info_callbacks = {
	.pairing_complete = pairing_complete,
	.addr_res_support_read = addr_res_support_read,
};

static void bond_addr_cb(const struct bt_bond_info *info, void *user_data)
{
	bt_addr_le_t *addr = user_data;

	bt_addr_le_copy(addr, &info->addr);
}

static void test_peripheral(bool expect_car_support)
{
	const struct bt_data ad[] = {
		BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR))
	};
	bt_addr_le_t peer = *BT_ADDR_LE_ANY;
	enum bt_le_addr_res_support support;
	struct bt_le_adv_param param;
	struct bt_le_ext_adv *adv;
	int err;

	bt_conn_cb_register(&conn_callbacks);

	err = bt_conn_auth_info_cb_register(&auth_info_callbacks);
	TEST_ASSERT(err == 0, "bt_conn_auth_info_cb_register failed (%d)", err);

	err = bt_enable(NULL);
	TEST_ASSERT(err == 0, "bt_enable failed (%d)", err);

	err = bt_le_adv_start(BT_LE_ADV_CONN_FAST_1, ad, ARRAY_SIZE(ad), NULL, 0);
	TEST_ASSERT(err == 0, "bt_le_adv_start failed (%d)", err);

	WAIT_FOR_FLAG(flag_connected);
	WAIT_FOR_FLAG(flag_bonded);

	WAIT_FOR_FLAG(flag_support_read);
	TEST_ASSERT(read_support == (expect_car_support ? BT_LE_ADDR_RES_SUPPORT_YES
							: BT_LE_ADDR_RES_SUPPORT_NO),
		    "Unexpected address resolution support %d reported", read_support);

	WAIT_FOR_FLAG_UNSET(flag_connected);

	bt_foreach_bond(BT_ID_DEFAULT, bond_addr_cb, &peer);
	TEST_ASSERT(!bt_addr_le_eq(&peer, BT_ADDR_LE_ANY), "No bond found");

	support = bt_le_bond_addr_res_support(BT_ID_DEFAULT, &peer);
	TEST_ASSERT(support == (expect_car_support ? BT_LE_ADDR_RES_SUPPORT_YES
						   : BT_LE_ADDR_RES_SUPPORT_NO),
		    "Unexpected address resolution support %d", support);

	param = *BT_LE_ADV_PARAM(BT_LE_ADV_OPT_CONN |
				 BT_LE_ADV_OPT_DIR_MODE_LOW_DUTY |
				 BT_LE_ADV_OPT_DIR_ADDR_RPA,
				 BT_GAP_ADV_FAST_INT_MIN_2,
				 BT_GAP_ADV_FAST_INT_MAX_2,
				 &peer);

	err = bt_le_adv_start(&param, NULL, 0, NULL, 0);

	if (expect_car_support) {
		TEST_ASSERT(err == 0,
			    "Directed advertising to a peer supporting address "
			    "resolution failed (%d)", err);

		err = bt_le_adv_stop();
		TEST_ASSERT(err == 0, "bt_le_adv_stop failed (%d)", err);
	} else {
		TEST_ASSERT(err == -ENOTSUP,
			    "Directed advertising to a peer without address "
			    "resolution was not refused with -ENOTSUP (%d)", err);
	}

	/* The same check applies when creating an extended advertising set */
	param.options |= BT_LE_ADV_OPT_EXT_ADV;

	err = bt_le_ext_adv_create(&param, NULL, &adv);
	if (expect_car_support) {
		TEST_ASSERT(err == 0, "bt_le_ext_adv_create failed (%d)", err);

		err = bt_le_ext_adv_delete(adv);
		TEST_ASSERT(err == 0, "bt_le_ext_adv_delete failed (%d)", err);
	} else {
		TEST_ASSERT(err == -ENOTSUP,
			    "Directed set creation was not refused with -ENOTSUP (%d)", err);
	}

	/* And when a set created without a target peer is updated to
	 * directed advertising.
	 */
	err = bt_le_ext_adv_create(BT_LE_EXT_ADV_CONN, NULL, &adv);
	TEST_ASSERT(err == 0, "bt_le_ext_adv_create failed (%d)", err);

	err = bt_le_ext_adv_update_param(adv, &param);
	if (expect_car_support) {
		TEST_ASSERT(err == 0, "bt_le_ext_adv_update_param failed (%d)", err);
	} else {
		TEST_ASSERT(err == -ENOTSUP,
			    "Directed param update was not refused with -ENOTSUP (%d)", err);
	}

	err = bt_le_ext_adv_delete(adv);
	TEST_ASSERT(err == 0, "bt_le_ext_adv_delete failed (%d)", err);

	TEST_PASS("Peripheral passed");
}

static void test_peripheral_car(void)
{
	test_peripheral(true);
}

static void test_peripheral_no_car(void)
{
	test_peripheral(false);
}

static const struct bst_test_instance test_inst[] = {
	{
		.test_id = "peripheral_car",
		.test_descr = "After bonding to a central that supports address resolution, "
			      "directed advertising with an RPA target address is allowed.",
		.test_main_f = test_peripheral_car,
	},
	{
		.test_id = "peripheral_no_car",
		.test_descr = "After bonding to a central without the Central Address "
			      "Resolution characteristic, directed advertising with an RPA "
			      "target address is refused.",
		.test_main_f = test_peripheral_no_car,
	},
	BSTEST_END_MARKER
};

struct bst_test_list *test_peripheral_install(struct bst_test_list *tests)
{
	return bst_add_tests(tests, test_inst);
}
