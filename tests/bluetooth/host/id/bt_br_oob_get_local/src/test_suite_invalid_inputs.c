/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "host_mocks/assert.h"
#include "testing_common_defs.h"

#include <zephyr/bluetooth/hci.h>
#include <zephyr/kernel.h>
#include <zephyr/ztest.h>

#include <host/hci_core.h>
#include <host/id.h>

ZTEST_SUITE(bt_br_oob_get_local_invalid_inputs, NULL, NULL, NULL, NULL, NULL);

/*
 *  Test passing NULL reference for the OOB reference argument
 *
 *  Constraints:
 *   - NULL reference is used as an argument for the OOB reference
 *
 *  Expected behaviour:
 *   - An assertion is raised and execution stops
 */
ZTEST(bt_br_oob_get_local_invalid_inputs, test_null_oob_reference)
{
	expect_assert();
	bt_br_oob_get_local(NULL);
}
