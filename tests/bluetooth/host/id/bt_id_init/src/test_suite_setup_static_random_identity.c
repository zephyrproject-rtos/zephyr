/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"
#include "mocks/hci_core.h"
#include "mocks/kernel.h"
#include "mocks/kernel_expects.h"
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

static void tc_setup(void *f)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_cmd_rsp_data, 0x00, sizeof(struct custom_bt_hci_rp_vs_read_static_addrs));

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_SUITE(bt_id_init_setup_static_random_identity, NULL, NULL, tc_setup, NULL, NULL);

static int bt_hci_cmd_send_sync_custom_fake(uint16_t opcode, struct net_buf *buf,
					    struct net_buf **rsp)
{
	const char *func_name = "bt_hci_cmd_send_sync";

	/* When bt_setup_public_id_addr() is called, this will make it returns 0 without changing
	 * bt_dev.id_count value
	 */
	if (opcode == BT_HCI_OP_READ_BD_ADDR) {
		return -1;
	}

	zassert_equal(opcode, BT_HCI_OP_VS_READ_STATIC_ADDRS,
		      "'%s()' was called with incorrect '%s' value", func_name, "opcode");
	zassert_is_null(buf, "'%s()' was called with incorrect '%s' value", func_name, "buf");
	zassert_not_null(rsp, "'%s()' was called with incorrect '%s' value", func_name, "rsp");

	*rsp = &hci_cmd_rsp;
	hci_cmd_rsp.data = (void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;

	return 0;
}

/*
 *  Test initializing the device identity with static random address by calling bt_id_init() while
 *  the device has no identity and bt_dev.id_count is set to 0.
 *  bt_setup_public_id_addr() should fail to setup the identity with public address, so the function
 *  should attempt to setup a static random identity.
 *
 *  Constraints:
 *   - bt_dev.id_count is set to 0
 *   - bt_setup_public_id_addr() returns 0 without setting up the device identity
 *   - bt_setup_random_id_addr() returns 0 (success)
 *   - set_random_address() returns 0 (success)
 *
 *  Expected behaviour:
 *   - bt_id_init() returns 0
 *   - bt_dev.id_count is set to 1
 */
ZTEST(bt_id_init_setup_static_random_identity, test_init_dev_identity_succeeds)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_SETTINGS);

	/* This part will make bt_setup_random_id_addr() returns 0 (success) and sets
	 * bt_dev.id_count value to 1
	 */
	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 1;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	/* This will make set_random_address() succeeds and returns 0 */
	bt_addr_copy(&bt_dev.random_addr.a, &BT_STATIC_RANDOM_LE_ADDR_1->a);

	err = bt_id_init();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);

	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

	zassert_mem_equal(&bt_dev.id_addr[0], BT_STATIC_RANDOM_LE_ADDR_1, sizeof(bt_addr_le_t),
			  "Incorrect address was set");

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_k_work_init_delayable(&bt_dev.rpa_update);
#endif
}

/*
 *  Test initializing the device identity with static random address by calling bt_id_init() while
 *  the device has no identity and bt_dev.id_count is set to 0.
 *  bt_setup_public_id_addr() should fail to setup the identity with public address, so the function
 *  should attempt to setup a static random identity which should fail as well when
 *  bt_setup_random_id_addr() is called.
 *
 *  Constraints:
 *   - bt_dev.id_count is set to 0
 *   - bt_setup_public_id_addr() returns 0 without setting up the device identity
 *   - bt_setup_random_id_addr() returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_init() returns a negative error code (failure)
 */
ZTEST(bt_id_init_setup_static_random_identity, test_init_dev_identity_bt_setup_random_id_addr_fails)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	/* This part will make bt_setup_random_id_addr() returns a negative number representing an
	 * execution error
	 */
	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 1;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	bt_rand_fake.return_val = -1;
	bt_smp_irk_get_fake.return_val = -1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_id_init();

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}

/*
 *  Test initializing the device identity with static random address by calling bt_id_init() while
 *  the device has no identity and bt_dev.id_count is set to 0.
 *  bt_setup_public_id_addr() should fail to setup the identity with public address, so the function
 *  should attempt to setup a static random identity which should fail as well when
 *  bt_setup_random_id_addr() is called.
 *
 * Constraints:
 *   - bt_dev.id_count is set to 0
 *   - bt_setup_public_id_addr() returns 0 without setting up the device identity
 *   - bt_setup_random_id_addr() returns 0 (success)
 *   - set_random_address() returns a negative error code (failure)
 *
 *  Expected behaviour:
 *   - bt_id_init() returns a negative error code (failure)
 */
ZTEST(bt_id_init_setup_static_random_identity, test_init_dev_identity_set_random_address_fails)
{
	int err;
	uint16_t *vs_commands = (uint16_t *)bt_dev.vs_commands;
	struct bt_hci_rp_vs_read_static_addrs *rp =
		(void *)&hci_cmd_rsp_data.hci_rp_vs_read_static_addrs;
	struct bt_hci_vs_static_addr *static_addr = (void *)&hci_cmd_rsp_data.hci_vs_static_addr;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_SETTINGS);

	/* This part will make bt_setup_random_id_addr() returns 0 (success) and sets
	 * bt_dev.id_count value to 1
	 */
	*vs_commands = (1 << BT_VS_CMD_BIT_READ_STATIC_ADDRS);

	rp->num_addrs = 1;
	bt_addr_copy(&static_addr[0].bdaddr, &BT_STATIC_RANDOM_LE_ADDR_1->a);

	hci_cmd_rsp.len = sizeof(struct bt_hci_rp_vs_read_static_addrs) +
			  rp->num_addrs * sizeof(struct bt_hci_vs_static_addr);

	/* This will make set_random_address() returns a negative number error code */
	bt_hci_cmd_create_fake.return_val = NULL;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_id_init();

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
