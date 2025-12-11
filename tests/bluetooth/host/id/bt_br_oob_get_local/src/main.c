/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/fff.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

DEFINE_FFF_GLOBALS;

static void fff_reset_rule_before(const struct ztest_unit_test *test, void *fixture)
{
	memset(&bt_dev, 0x00, sizeof(struct bt_dev));
}

ZTEST_RULE(fff_reset_rule, fff_reset_rule_before, NULL);

ZTEST_SUITE(bt_br_oob_get_local, NULL, NULL, NULL, NULL, NULL);

/*
 *  Get BR/EDR local Out Of Band information
 *
 *  Constraints:
 *   - Use a valid reference
 *
 *  Expected behaviour:
 *   - Address is copied to the passed OOB reference
 */
ZTEST(bt_br_oob_get_local, test_get_local_out_of_band_information)
{
	int err;
	struct bt_br_oob oob;

	bt_addr_le_copy(&bt_dev.id_addr[0], BT_RPA_LE_ADDR);

	err = bt_br_oob_get_local(&oob);

	zassert_ok(err, "Unexpected error code '%d' was returned", err);
	zassert_mem_equal(&oob.addr, &BT_RPA_LE_ADDR->a, sizeof(bt_addr_t),
			  "Incorrect address was set");
}
