/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/keys.h"
#include "mocks/keys_expects.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

/* Hold data representing HCI command response for command BT_HCI_OP_READ_BD_ADDR */
static struct bt_keys resolving_key;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
	memset(&resolving_key, 0x00, sizeof(struct bt_keys));

	KEYS_FFF_FAKES_LIST(RESET_FAKE);
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_lookup_id_addr, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test returning the same address pointer passed when 'CONFIG_BT_SMP' isn't enabled
 *
 *  Constraints:
 *   - A valid ID (< CONFIG_BT_ID_MAX) is used
 *   - A valid address reference is used
 *   - 'CONFIG_BT_SMP' isn't enabled
 *
 *  Expected behaviour:
 *   - bt_lookup_id_addr() returns the same address pointer used as an argument
 */
ZTEST(bt_lookup_id_addr, test_config_bt_smp_not_enabled)
{
	uint8_t id = 0x00;
	const bt_addr_le_t *addr = BT_RPA_LE_ADDR;
	const bt_addr_le_t *returned_addr;

	Z_TEST_SKIP_IFDEF(CONFIG_BT_SMP);

	returned_addr = bt_lookup_id_addr(id, addr);

	expect_not_called_bt_keys_find_irk();

	zassert_true(returned_addr == addr, "Incorrect address was returned");
}

/*
 *  Test returning the same address pointer passed when 'CONFIG_BT_SMP' is enabled, but the
 *  address couldn't be resolved by bt_keys_find_irk().
 *
 *  Constraints:
 *   - A valid ID (< CONFIG_BT_ID_MAX) is used
 *   - A valid address reference is used
 *   - bt_keys_find_irk() returns NULL (which represents that address couldn't be resolved)
 *   - 'CONFIG_BT_SMP' is enabled
 *
 *  Expected behaviour:
 *   - bt_lookup_id_addr() returns the same address pointer used as an argument
 */
ZTEST(bt_lookup_id_addr, test_config_bt_smp_enabled_address_resolving_fails)
{
	uint8_t id = 0x00;
	const bt_addr_le_t *addr = BT_RPA_LE_ADDR;
	const bt_addr_le_t *returned_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);

	bt_keys_find_irk_fake.return_val = NULL;

	returned_addr = bt_lookup_id_addr(id, addr);

	expect_single_call_bt_keys_find_irk(id, addr);

	zassert_true(returned_addr == addr, "Incorrect address was returned");
}

static struct bt_keys *bt_keys_find_irk_custom_fake(uint8_t id, const bt_addr_le_t *addr)
{
	return &resolving_key;
}

/*
 *  Test returning a pointer to the resolved address for the address passed.
 *  'CONFIG_BT_SMP' is enabled and address is resolved by bt_keys_find_irk().
 *
 *  Constraints:
 *   - A valid ID (< CONFIG_BT_ID_MAX) is used
 *   - A valid address reference is used
 *   - bt_keys_find_irk() returns a valid key reference
 *   - 'CONFIG_BT_SMP' is enabled
 *
 *  Expected behaviour:
 *   - bt_lookup_id_addr() returns the resolved address instead of the input address
 */
ZTEST(bt_lookup_id_addr, test_config_bt_smp_enabled_address_resolving_succeeds)
{
	uint8_t id = 0x00;
	const bt_addr_le_t *addr = BT_RPA_LE_ADDR;
	const bt_addr_le_t *returned_addr;

	Z_TEST_SKIP_IFNDEF(CONFIG_BT_SMP);

	bt_keys_find_irk_fake.custom_fake = bt_keys_find_irk_custom_fake;

	returned_addr = bt_lookup_id_addr(id, addr);

	expect_single_call_bt_keys_find_irk(id, addr);

	zassert_true(returned_addr == &resolving_key.addr, "Incorrect address was returned");
}
