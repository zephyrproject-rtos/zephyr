/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	(void)memset(&bt_dev, 0x00, sizeof(struct bt_dev));

	HCI_CORE_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_id_save_adv_addr, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test recording the advertising address for a set advertising with a public
 *  identity address.
 *
 *  Constraints:
 *   - Own address type is BT_HCI_OWN_ADDR_PUBLIC
 *
 *  Expected behaviour:
 *   - Advertising address is loaded with the identity address of the set
 */
ZTEST(bt_id_save_adv_addr, test_public_identity_address)
{
	struct bt_le_ext_adv adv = {0};

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR);

	bt_id_save_adv_addr(&adv, BT_HCI_OWN_ADDR_PUBLIC);

	zassert_mem_equal(&adv.adv_addr, BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

/*
 *  Test recording the advertising address for a set advertising with a public
 *  identity address as the fallback of controller-based address resolution.
 *
 *  Constraints:
 *   - Own address type is BT_HCI_OWN_ADDR_RPA_OR_PUBLIC
 *
 *  Expected behaviour:
 *   - Advertising address is loaded with the identity address of the set
 */
ZTEST(bt_id_save_adv_addr, test_rpa_or_public_identity_address)
{
	struct bt_le_ext_adv adv = {0};

	bt_addr_le_copy(&bt_dev.id_addr[BT_ID_DEFAULT], BT_LE_ADDR);

	bt_id_save_adv_addr(&adv, BT_HCI_OWN_ADDR_RPA_OR_PUBLIC);

	zassert_mem_equal(&adv.adv_addr, BT_LE_ADDR, sizeof(bt_addr_le_t),
			  "Incorrect address was set");
}

/*
 *  Test recording the advertising address for a set advertising with a random
 *  address on a controller without the extended advertising feature. This
 *  covers a device-wide random address that was not set through
 *  bt_id_set_adv_random_addr(), e.g. an RPA programmed through
 *  bt_id_set_private_addr() when privacy is enabled.
 *
 *  Constraints:
 *   - Own address type is BT_HCI_OWN_ADDR_RANDOM
 *   - The controller extended advertising feature bit isn't set
 *
 *  Expected behaviour:
 *   - Advertising address is loaded with the device-wide random address
 */
ZTEST(bt_id_save_adv_addr, test_random_address_no_ext_adv)
{
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFDEF(CONFIG_BT_EXT_ADV);

	bt_addr_copy(&bt_dev.random_addr, BT_RPA_ADDR);

	bt_id_save_adv_addr(&adv, BT_HCI_OWN_ADDR_RANDOM);

	zassert_equal(adv.adv_addr.type, BT_ADDR_LE_RANDOM, "Incorrect address type was set");
	zassert_mem_equal(&adv.adv_addr.a, BT_RPA_ADDR, sizeof(bt_addr_t),
			  "Incorrect address was set");
}

/*
 *  Test recording the advertising address for a set advertising with a random
 *  address on a controller with the extended advertising feature. The per-set
 *  random address is recorded when it is programmed through
 *  bt_id_set_adv_random_addr(), so recording must leave it untouched.
 *
 *  Constraints:
 *   - Own address type is BT_HCI_OWN_ADDR_RANDOM
 *   - The controller extended advertising feature bit is set
 *
 *  Expected behaviour:
 *   - Advertising address is left as it was
 */
ZTEST(bt_id_save_adv_addr, test_random_address_ext_adv)
{
	struct bt_le_ext_adv adv = {0};

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_EXT_ADV);

	bt_addr_le_copy(&adv.adv_addr, BT_RPA_LE_ADDR);

	/* Set the extended advertising feature bit, the setter equivalent of
	 * what BT_LE_FEAT_TEST() reads.
	 */
	bt_dev.le.features[(BT_LE_FEAT_BIT_EXT_ADV) >> 3] |= BIT((BT_LE_FEAT_BIT_EXT_ADV) & 7);

	bt_addr_copy(&bt_dev.random_addr, BT_ADDR);

	bt_id_save_adv_addr(&adv, BT_HCI_OWN_ADDR_RANDOM);

	zassert_mem_equal(&adv.adv_addr, BT_RPA_LE_ADDR, sizeof(bt_addr_le_t),
			  "Address was unexpectedly overwritten");
}
