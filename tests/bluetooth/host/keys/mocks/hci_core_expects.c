/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/bluetooth/buf.h>
#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"

void expect_single_call_bt_unpair(uint8_t id, const bt_addr_le_t *addr)
{
	const char *func_name = "bt_unpair";

	zassert_equal(bt_unpair_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_unpair_fake.arg0_val, id,
			  "'%s()' was called with incorrect '%s' value", func_name, "id");
	zassert_mem_equal(bt_unpair_fake.arg1_val, addr, sizeof(bt_addr_le_t),
			  "'%s()' was called with incorrect '%s' value", func_name, "addr");
}

void expect_not_called_bt_unpair(void)
{
	const char *func_name = "bt_unpair";

	zassert_equal(bt_unpair_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
