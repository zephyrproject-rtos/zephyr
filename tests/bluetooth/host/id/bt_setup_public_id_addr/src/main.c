/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

/* Hold data representing HCI command response for command BT_HCI_OP_READ_BD_ADDR */
static struct net_buf hci_cmd_rsp;

/* Hold data representing response data for HCI command BT_HCI_OP_READ_BD_ADDR */
static struct bt_hci_rp_read_bd_addr hci_rp_read_bd_addr;

static void tc_setup(void *f)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_rp_read_bd_addr, 0x00, sizeof(struct bt_hci_rp_read_bd_addr));

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

/* Default test cases to run under all configuration */
ZTEST_SUITE(bt_setup_public_id_addr_default, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Test reading controller public address fails
 *
 *  Constraints:
 *   - bt_id_read_public_addr() returns zero
 *
 *  Expected behaviour:
 *   - ID count is set to 0 and bt_setup_public_id_addr() returns 0
 */
ZTEST(bt_setup_public_id_addr_default, test_bt_id_read_public_addr_returns_zero)
{
	int err;

	/* This will force bt_id_read_public_addr() to fail */
	bt_hci_cmd_send_sync_fake.return_val = 1;

	err = bt_setup_public_id_addr();

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
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *
 *  Expected behaviour:
 *   - Return value is 0
 *   - Public address is loaded to bt_dev.id_addr[]
 */
ZTEST(bt_setup_public_id_addr_default, test_bt_id_read_public_addr_returns_valid_id_count)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_public_id_addr();

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);
}
