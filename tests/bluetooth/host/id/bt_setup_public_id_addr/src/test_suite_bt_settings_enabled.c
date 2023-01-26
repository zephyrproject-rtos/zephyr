/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/settings.h"
#include "mocks/settings_expects.h"
#include "mocks/smp.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

/* Hold data representing HCI command response for command BT_HCI_OP_READ_BD_ADDR */
static struct net_buf hci_cmd_rsp;

/* Hold data representing response data for HCI command BT_HCI_OP_READ_BD_ADDR */
static struct bt_hci_rp_read_bd_addr hci_rp_read_bd_addr;

static void tc_setup(void *f)
{
	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SETTINGS);

	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_rp_read_bd_addr, 0x00, sizeof(struct bt_hci_rp_read_bd_addr));

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
	SETTINGS_FFF_FAKES_LIST(RESET_FAKE);

	if (IS_ENABLED(CONFIG_BT_PRIVACY)) {
		SMP_FFF_FAKES_LIST(RESET_FAKE);
	}
}

ZTEST_SUITE(bt_setup_public_id_addr_bt_settings_enabled, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Test reading controller public address fails and no attempt to store settings.
 *
 *  Constraints:
 *   - bt_id_read_public_addr() returns zero
 *
 *  Expected behaviour:
 *   - ID count is set to 0 and bt_setup_public_id_addr() returns 0
 *   - No expected calls to bt_settings_save_id()
 */
ZTEST(bt_setup_public_id_addr_bt_settings_enabled, test_bt_id_read_public_addr_returns_zero)
{
	int err;

	/* This will force bt_id_read_public_addr() to fail */
	bt_hci_cmd_send_sync_fake.return_val = 1;

	err = bt_setup_public_id_addr();

	expect_not_called_bt_settings_save_id();

	zassert_true(bt_dev.id_count == 0, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);
	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
}

static int bt_hci_cmd_send_sync_custom_fake(uint16_t opcode, struct net_buf *buf,
					    struct net_buf **rsp)
{
	const char *func_name = "bt_hci_cmd_send_sync";

	zassert_equal(opcode, BT_HCI_OP_READ_BD_ADDR, "'%s()' was called with incorrect '%s' value",
		      func_name, "opcode");
	zassert_is_null(buf, "'%s()' was called with incorrect '%s' value", func_name, "buf");
	zassert_not_null(rsp, "'%s()' was called with incorrect '%s' value", func_name, "rsp");

	*rsp = &hci_cmd_rsp;
	hci_cmd_rsp.data = (void *)&hci_rp_read_bd_addr;

	return 0;
}

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync().
 *  Even if the operation succeeded, bt_settings_save_id() shouldn't be called to
 *  store settings as the 'BT_DEV_READY' bit isn't set.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - BT_DEV_READY bit isn't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - Return value is 0
 *   - Public address is loaded to bt_dev.id_addr[]
 *   - No expected calls to bt_settings_save_id()
 */
ZTEST(bt_setup_public_id_addr_bt_settings_enabled,
	  test_bt_id_read_public_addr_succeeds_bt_dev_ready_cleared)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	atomic_clear_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_setup_public_id_addr();

	expect_not_called_bt_settings_save_id();

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);
}

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync().
 *  With the 'BT_DEV_READY' bit set, bt_settings_save_id() should be called to store
 *  settings to persistent memory.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - BT_DEV_READY bit isn't set in bt_dev.flags
 *
 *  Expected behaviour:
 *   - Return value is 0
 *   - Public address is loaded to bt_dev.id_addr[]
 *   - No expected calls to bt_settings_save_id()
 */
ZTEST(bt_setup_public_id_addr_bt_settings_enabled,
	  test_bt_id_read_public_addr_succeeds_bt_dev_ready_set)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	atomic_set_bit(bt_dev.flags, BT_DEV_READY);

	err = bt_setup_public_id_addr();

	expect_single_call_bt_settings_save_id();

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);
}

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync().
 *  'BT_DEV_STORE_ID' should be set when IRK isn't set.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - CONFIG_BT_PRIVACY is enabled
 *
 *  Expected behaviour:
 *   - Return value is 0
 *   - 'BT_DEV_STORE_ID' bit is set inside bt_dev.flags
 */
ZTEST(bt_setup_public_id_addr_bt_settings_enabled, test_store_flag_set_correctly)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_smp_irk_get_fake.return_val = 1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_public_id_addr();

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	zassert_true(atomic_test_bit(bt_dev.flags, BT_DEV_STORE_ID),
		     "Flags were not correctly set");
}
