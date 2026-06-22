/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/net_buf.h"
#include "mocks/net_buf_expects.h"
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
	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_id_read_public_addr, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync(), but
 *  operation fails and a non-success error code is returned.
 *
 *  Constraints:
 *   - A valid address reference is passed to bt_id_read_public_addr()
 *   - bt_hci_cmd_send_sync() returns a non-zero error code
 *
 *  Expected behaviour:
 *   - Execution stops and a zero return value is returned which represents failure
 */
ZTEST(bt_id_read_public_addr, test_bt_hci_cmd_send_sync_returns_err)
{
	int err;
	bt_addr_le_t addr;

	bt_hci_cmd_send_sync_fake.return_val = 1;

	err = bt_id_read_public_addr(&addr);

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR);
	expect_not_called_net_buf_unref();

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
 *  Although, bt_hci_cmd_send_sync() return value is success but response data contains
 *  an invalid BT address
 *
 *  Constraints:
 *   - A valid address reference is passed to bt_id_read_public_addr()
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains invalid address
 *
 *  Expected behaviour:
 *   - Execution stops and a zero return value is returned which represents failure
 */
ZTEST(bt_id_read_public_addr, test_bt_hci_cmd_send_sync_response_has_invalid_bt_address)
{
	int err;
	bt_addr_le_t addr;
	const bt_addr_t *invalid_bt_address[] = {BT_ADDR_ANY, BT_ADDR_NONE};
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	for (size_t i = 0; i < ARRAY_SIZE(invalid_bt_address); i++) {

		memset(&bt_dev, 0x00, sizeof(struct bt_dev));
		memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
		memset(&hci_rp_read_bd_addr, 0x00, sizeof(struct bt_hci_rp_read_bd_addr));
		NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
		HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);

		bt_addr_copy(&rp->bdaddr, invalid_bt_address[i]);
		bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

		err = bt_id_read_public_addr(&addr);

		expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR);
		expect_single_call_net_buf_unref(&hci_cmd_rsp);

		zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	}
}

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *
 *  Constraints:
 *   - A valid address reference is passed to bt_id_read_public_addr()
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *
 *  Expected behaviour:
 *   - Return value is success
 */
ZTEST(bt_id_read_public_addr, test_bt_hci_cmd_send_sync_response_has_valid_bt_address)
{
	int err;
	bt_addr_le_t addr;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	bt_addr_copy(&rp->bdaddr, &BT_LE_ADDR->a);
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_id_read_public_addr(&addr);

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_READ_BD_ADDR);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err == 1, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&addr, BT_LE_ADDR, sizeof(bt_addr_le_t), "Incorrect address was set");
}
