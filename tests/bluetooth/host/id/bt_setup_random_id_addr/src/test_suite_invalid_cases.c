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
#include <zephyr/bluetooth/hci_vs.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

/* Hold data representing HCI command response for command BT_HCI_OP_VS_READ_STATIC_ADDRS */
static struct net_buf hci_cmd_rsp;

/* Hold data representing response data for HCI command BT_HCI_OP_VS_READ_STATIC_ADDRS */
static struct custom_bt_hci_rp_vs_read_static_addrs {
	struct bt_hci_rp_vs_read_static_addrs hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr hci_vs_static_addr[CONFIG_BT_ID_MAX];
} __packed hci_cmd_rsp_data;

static void tc_setup(void *f)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_cmd_rsp_data, 0x00, sizeof(struct custom_bt_hci_rp_vs_read_static_addrs));

	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_setup_random_id_addr_invalid_cases, NULL, NULL, tc_setup, NULL, NULL);

/*
 *  Testing setting up device random address while VS command for reading static address isn't
 *  enabled, so reading static address fails.
 *
 *  Constraints:
 *   - VS command for reading static address isn't enabled
 *   - No identity exists and bt_dev.id_count equals 0
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_vs_reading_static_address_fails)
{
	int err;

	bt_dev.id_count = 0;

	err = bt_setup_random_id_addr();

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() fails and returns a non-success error code.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns a non-zero error code
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_bt_hci_cmd_send_sync_returns_err)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);
	bt_hci_cmd_send_sync_fake.return_val = 1;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_not_called_net_buf_unref();

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}

static int bt_hci_cmd_send_sync_custom_fake(uint16_t opcode, struct net_buf *buf,
					    struct net_buf **rsp)
{
	const char *func_name = "bt_hci_cmd_send_sync";

	zassert_equal(opcode, BT_HCI_OP_VS_READ_STATIC_ADDRS,
		      "'%s()' was called with incorrect '%s' value", func_name, "opcode");
	zassert_is_null(buf, "'%s()' was called with incorrect '%s' value", func_name, "buf");
	zassert_not_null(rsp, "'%s()' was called with incorrect '%s' value", func_name, "rsp");

	*rsp = &hci_cmd_rsp;
	hci_cmd_rsp.data = (void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;

	return 0;
}

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() returns 0 (success) and the returned number of addresses is 0.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  The response size set is less than the expected response size,so response should be
 *  discarded and operation fails.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response header size is less than expected
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_bt_hci_cmd_send_sync_response_incomplete_1)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HCI_VS_EXT_DETECT);

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) - 1;
	rp->num_addrs = 0;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() returns 0 (success) and the returned number of addresses is 1, but no
 *  actual data exists in the response.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  The response size set is less than the expected response header size plus the size of the data
 *  holding the returned addresses information, so response should be discarded and operation fails.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response size is less than expected
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_bt_hci_cmd_send_sync_response_incomplete_2)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HCI_VS_EXT_DETECT);

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs);
	rp->num_addrs = 1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() returns 0 (success) and the returned number of addresses is 0.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  Even if the response size follows the formula, as the returned addresses
 *  count is 0, the response should be discarded and operation fails as the returned number of
 *  addresses is 0.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response contains no addresses and count is set to 0
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_bt_hci_cmd_send_sync_response_zero_id_addresses)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_HCI_VS_EXT_DETECT);

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs);
	rp->num_addrs = 0;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Testing setting up device random address while there is an identity exists.
 *
 *  Constraints:
 *   - An identity exists and bt_dev.id_count > 0
 *
 *  Expected behaviour:
 *   - A negative error code is returned by bt_id_create().
 */
ZTEST(bt_setup_random_id_addr_invalid_cases, test_set_up_random_address_fails_when_identity_exists)
{
	int err;

	bt_dev.id_count = 1;

	err = bt_setup_random_id_addr();

	zassert_true(err != 0, "Unexpected error code '%d' was returned", err);
}
