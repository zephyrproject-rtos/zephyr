/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/id.h>

ZTEST_SUITE(bt_id_read_public_addr_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test reading controller public address through bt_hci_cmd_send_sync() with NULL address.
 *
 *  Constraints:
 *   - Address is passed as NULL
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_id_read_public_addr_invalid_inputs, test_null_device_address)
{
	expect_assert();
	bt_id_read_public_addr(NULL);
}
