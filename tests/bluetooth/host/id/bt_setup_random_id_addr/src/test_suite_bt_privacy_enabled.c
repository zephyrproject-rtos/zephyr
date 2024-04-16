/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#if defined(CONFIG_BT_PRIVACY)

#include "mocks/crypto.h"
#include "mocks/crypto_expects.h"
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"
#include "mocks/smp.h"
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

static uint8_t testing_irk_value[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
					0x08, 0x09, 0x10, 0x11, 0x12, 0x13, 0x14, 0x15};

static void tc_setup(void *f)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_cmd_rsp_data, 0x00, sizeof(struct custom_bt_hci_rp_vs_read_static_addrs));

	SMP_FFF_FAKES_LIST(RESET_FAKE);
	CRYPTO_FFF_FAKES_LIST(RESET_FAKE);
	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_setup_random_id_addr_privacy_enabled, NULL, NULL, tc_setup, NULL, NULL);

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
 *  Test reading controller random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *  IRK isn't set by bt_smp_irk_get() and bt_rand() succeeds.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - bt_smp_irk_get() returns an error
 *   - bt_rand() succeeds
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *
 *  Expected behaviour:
 *   - Return value is 0
 */
ZTEST(bt_setup_random_id_addr_privacy_enabled, test_create_id_irk_null)
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

	bt_rand_fake.return_val = 0;
	bt_smp_irk_get_fake.return_val = -1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_rand(&bt_dev.irk[0], 16);

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test reading controller random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *  As IRK isn't set by bt_smp_irk_get() and bt_rand() fails, an error is returned.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - bt_smp_irk_get() returns an error
 *   - bt_rand() returns an error
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *
 *  Expected behaviour:
 *   - Return value isn't 0 and equal to the error returned by bt_rand()
 */
ZTEST(bt_setup_random_id_addr_privacy_enabled, test_create_id_irk_null_bt_rand_fails)
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

	bt_rand_fake.return_val = -1;
	bt_smp_irk_get_fake.return_val = -1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_rand(&bt_dev.irk[0], 16);

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

static int bt_smp_irk_get_fill_zero_irk_custom_fake(uint8_t *ir, uint8_t *irk)
{
	__ASSERT_NO_MSG(ir != NULL);
	__ASSERT_NO_MSG(irk != NULL);

	memset(irk, 0x00, 16);

	return 0;
}

/*
 *  Test reading controller random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *  IRK is set by bt_smp_irk_get() and bt_rand() succeeds.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - bt_smp_irk_get() succeeds
 *   - bt_rand() succeeds
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *
 *  Expected behaviour:
 *   - Return value is 0
 */
ZTEST(bt_setup_random_id_addr_privacy_enabled, test_create_id_irk_not_null_but_cleared)
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

	bt_rand_fake.return_val = 0;
	bt_smp_irk_get_fake.custom_fake = bt_smp_irk_get_fill_zero_irk_custom_fake;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_single_call_bt_rand(&bt_dev.irk[0], 16);

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
}

static int bt_smp_irk_get_non_zero_irk_custom_fake(uint8_t *ir, uint8_t *irk)
{
	__ASSERT_NO_MSG(ir != NULL);
	__ASSERT_NO_MSG(irk != NULL);

	memcpy(irk, testing_irk_value, 16);

	return 0;
}

/*
 *  Test reading controller random address through bt_hci_cmd_send_sync().
 *  bt_hci_cmd_send_sync() return value is success and response data contains a valid BT address.
 *  IRK is set by bt_smp_irk_get() and non-zero filled IRK is loaded.
 *
 *  Constraints:
 *   - bt_hci_cmd_send_sync() returns zero
 *   - Response data contains a valid address
 *   - bt_smp_irk_get() succeeds
 *   - 'CONFIG_BT_PRIVACY' is enabled
 *
 *  Expected behaviour:
 *   - Return value is 0
 *   - IRK is loaded to bt_dev.irk[]
 */
ZTEST(bt_setup_random_id_addr_privacy_enabled, test_create_id_irk_not_null_and_filled)
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

	bt_smp_irk_get_fake.custom_fake = bt_smp_irk_get_non_zero_irk_custom_fake;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_setup_random_id_addr();

	expect_not_called_bt_rand();

	zassert_true(err == 0, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&bt_dev.irk[0], testing_irk_value, sizeof(testing_irk_value),
			  "Incorrect IRK value was set");
}
#endif
