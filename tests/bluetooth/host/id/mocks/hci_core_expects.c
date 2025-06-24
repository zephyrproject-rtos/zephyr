/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>

void expect_single_call_bt_unpair(uint8_t id, const bt_addr_le_t *addr)
{
	const char *func_name = "bt_unpair";

	zassert_equal(bt_unpair_fake.call_count, 1, "'%s()' was called more than once", func_name);

	zassert_equal(bt_unpair_fake.arg0_val, id, "'%s()' was called with incorrect '%s' value",
		      func_name, "id");

	if (addr) {
		zassert_mem_equal(bt_unpair_fake.arg1_val, addr, sizeof(bt_addr_le_t),
				  "'%s()' was called with incorrect '%s' value", func_name, "addr");
	} else {
		zassert_equal(bt_unpair_fake.arg1_val, addr,
			      "'%s()' was called with incorrect '%s' value", func_name, "addr");
	}
}

void expect_not_called_bt_unpair(void)
{
	const char *func_name = "bt_unpair";

	zassert_equal(bt_unpair_fake.call_count, 0, "'%s()' was called unexpectedly", func_name);
}

void expect_single_call_bt_hci_cmd_alloc(void)
{
	const char *func_name = "bt_hci_cmd_alloc";

	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);
}

void expect_not_called_bt_hci_cmd_alloc(void)
{
	const char *func_name = "bt_hci_cmd_alloc";

	zassert_equal(bt_hci_cmd_alloc_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}

void expect_single_call_bt_hci_cmd_send_sync(uint16_t opcode)
{
	const char *func_name = "bt_hci_cmd_send_sync";

	zassert_equal(bt_hci_cmd_send_sync_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_hci_cmd_send_sync_fake.arg0_val, opcode,
		      "'%s()' was called with incorrect '%s' value", func_name, "opcode");
}

void expect_not_called_bt_hci_cmd_send_sync(void)
{
	const char *func_name = "bt_hci_cmd_send_sync";

	zassert_equal(bt_hci_cmd_send_sync_fake.call_count, 0, "'%s()' was called unexpectedly",
		      func_name);
}
