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
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

/* Hold data representing HCI command response for command BT_HCI_OP_VS_READ_STATIC_ADDRS */
static struct net_buf hci_cmd_rsp;

/* Hold data representing response data for HCI command BT_HCI_OP_VS_READ_STATIC_ADDRS */
static struct custom_bt_hci_rp_vs_read_static_addrs {
	struct bt_hci_rp_vs_read_static_addrs hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr hci_vs_static_addr[CONFIG_BT_ID_MAX];
} __packed hci_cmd_rsp_data;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

static void tc_setup(void *f)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_cmd_rsp_data, 0x00, sizeof(struct custom_bt_hci_rp_vs_read_static_addrs));

	NET_BUF_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_setup_random_id_addr, NULL, NULL, tc_setup, NULL, NULL);

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
 *  bt_hci_cmd_send_sync() returns 0 (success), the returned number of addresses
 *  is 1, and actual addresses information exists in the response.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  Response length is properly configured, and response data contains a valid identity
 *  information.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response contains single identity address
 *
 *  Expected behaviour:
 *   - Non-zero positive number equals to the number of addresses in the response
 */
ZTEST(bt_setup_random_id_addr, test_bt_hci_cmd_send_sync_returns_single_identity)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 1;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);

	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

#if CONFIG_BT_ID_MAX > 1

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() returns 0 (success), the returned number of addresses
 *  is 2, and actual addresses information exists in the response.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  Response length is properly configured, and response data contains a mixed data with
 *  2 identities.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response contains multiple identities, but only one is a static
 *     random address and the other should be discarded
 *
 *  Expected behaviour:
 *   - Non-zero positive number equals to the number of addresses in the response
 */
ZTEST(bt_setup_random_id_addr, test_bt_hci_cmd_send_sync_returns_single_valid_identity)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 2;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);
	bt_addr_copy(&static_addr[1].bdaddr, BT_ADDR);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);

	zassert_true(bt_dev.id_count == 2, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_mem_equal(&bt_dev.id_addr[1].a, BT_ADDR, sizeof(bt_addr_t),
			  "Incorrect address was set");
	zassert_equal(bt_dev.id_addr[1].type, BT_ADDR_LE_RANDOM, "Incorrect address was set");
}

/*
 *  Test reading controller static random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() returns 0 (success), the returned number of addresses
 *  is 2, and actual addresses information exists in the response.
 *
 *  Response size should follow the formula
 *  hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
 *            rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);
 *
 *  Response length is properly configured, and response data contains 2 valid identities.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns 0 (success)
 *   - bt_hci_cmd_send_sync() response contains multiple identity addresses
 *
 *  Expected behaviour:
 *   - Non-zero positive number equals to the number of addresses in the response
 */
ZTEST(bt_setup_random_id_addr, test_bt_hci_cmd_send_sync_returns_multiple_identities)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 2;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);
	bt_addr_copy(&static_addr[1].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_2->a);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_hci_cmd_send_sync(BT_HCI_OP_VS_READ_STATIC_ADDRS);
	expect_single_call_net_buf_unref(&hci_cmd_rsp);

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);

	zassert_true(bt_dev.id_count == 2, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_mem_equal(&bt_dev.id_addr[1], BT_STATIC_RANDOM_LE_ADDR_2, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}
#endif /* CONFIG_BT_ID_MAX > 1 */
