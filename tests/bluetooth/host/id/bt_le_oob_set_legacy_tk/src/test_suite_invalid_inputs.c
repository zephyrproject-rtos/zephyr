/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>
#include <mocks/conn.h>

ZTEST_SUITE(bt_le_oob_set_legacy_tk_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the connection reference argument
 *
 *  Constraints:
 *   - NULL reference is used for the connection reference
 *   - Valid reference is used for the 'TK' reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_oob_set_legacy_tk_invalid_inputs, test_null_conn_reference)
{
	const uint8_t tk[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
				0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

	expect_assert();
	bt_le_oob_set_legacy_tk(NULL, tk);
}

/*
 *  Test passing NULL reference for the 'TK' reference argument
 *
 *  Constraints:
 *   - Valid reference is used for the connection reference
 *   - NULL reference is used as an argument for the 'TK' reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_le_oob_set_legacy_tk_invalid_inputs, test_null_tk_reference)
{
	struct bt_conn conn = {0};

	expect_assert();
	bt_le_oob_set_legacy_tk(&conn, NULL);
}
