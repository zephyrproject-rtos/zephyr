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
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&hci_cmd_rsp, 0x00, sizeof(struct net_buf));
	memset(&hci_rp_read_bd_addr, 0x00, sizeof(struct bt_hci_rp_read_bd_addr));
}

ZTEST_SUITE(bt_id_init_setup_public_identity, NULL, NULL, tc_setup, NULL, NULL);

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
 *  Test initializing the device identity with public address by calling bt_id_init() while the
 *  device has no identity and bt_dev.id_count is set to 0.
 *  bt_setup_public_id_addr() should return 0 (success).
 *
 *  Constraints:
 *   - bt_dev.id_count is set to 0
 *   - bt_setup_public_id_addr() succeeds and returns 0
 *
 *  Expected behaviour:
 *   - bt_id_init() returns 0
 *   - bt_dev.id_count is set to 1
 */
ZTEST(bt_id_init_setup_public_identity, test_init_dev_identity_succeeds)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_SETTINGS);

	err = bt_id_init();

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
	zassert_true(bt_dev.id_count == 1, "Incorrect value '%d' was set to bt_dev.id_count",
		     bt_dev.id_count);

#if defined(CONFIG_BT_PRIVACY)
	expect_single_call_k_work_init_delayable(&bt_dev.rpa_update);
#endif
}

/*
 *  Test initializing the device identity with public address by calling bt_id_init() while the
 *  device has no identity and bt_dev.id_count is set to 0.
 *  bt_setup_public_id_addr() should return a negative value (failure).
 *
 *  Constraints:
 *   - bt_dev.id_count is set to 0
 *   - bt_setup_public_id_addr() fails in setting up device identity
 *
 *  Expected behaviour:
 *   - bt_id_init() returns a negative error code (failure)
 */
ZTEST(bt_id_init_setup_public_identity, test_init_dev_identity_fails)
{
	int err;
	struct bt_hci_rp_read_bd_addr *rp = (void *)&hci_rp_read_bd_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_PRIVACY);

	bt_addr_copy(&rp->bdaddr, BT_ADDR);
	bt_rand_fake.return_val = -1;
	bt_smp_irk_get_fake.return_val = -1;
	bt_hci_cmd_send_sync_fake.custom_fake = bt_hci_cmd_send_sync_custom_fake;

	err = bt_id_init();

	zassert_true(err < 0, "Unexpected error code '%d' was returned", err);
}
