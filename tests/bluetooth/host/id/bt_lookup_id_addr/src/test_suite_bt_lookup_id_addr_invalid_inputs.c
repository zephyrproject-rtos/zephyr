/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_lookup_id_addr_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test function with invalid ID ( >= CONFIG_BT_ID_MAX)
 *
 *  Constraints:
 *   - An invalid ID value is used
 *   - A valid address reference is used
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_lookup_id_addr_invalid_inputs, test_invalid_id_address)
{
	const bt_addr_le_t *addr = BT_RPA_LE_ADDR;

	expect_assert();
	bt_lookup_id_addr(CONFIG_BT_ID_MAX, addr);
}

/*
 *  Test function with NULL address
 *
 *  Constraints:
 *   - Any ID value can be used
 *   - Address is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion fails and execution stops
 */
ZTEST(bt_lookup_id_addr_invalid_inputs, test_null_device_address)
{
	expect_assert();
	bt_lookup_id_addr(0x00, NULL);
}
