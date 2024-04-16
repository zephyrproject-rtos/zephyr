/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/hci_core.h"
#include "mocks/hci_core_expects.h"

#include <zephyr/bluetooth/buf.h>
#include <zephyr/kernel.h>

void expect_single_call_bt_hci_cmd_create(uint16_t opcode, uint8_t param_len)
{
	const char *func_name = "bt_hci_cmd_create";

	zassert_equal(bt_hci_cmd_create_fake.call_count, 1, "'%s()' was called more than once",
		      func_name);

	zassert_equal(bt_hci_cmd_create_fake.arg0_val, opcode,
		      "'%s()' was called with incorrect '%s' value", func_name, "opcode");
	zassert_equal(bt_hci_cmd_create_fake.arg1_val, param_len,
		      "'%s()' was called with incorrect '%s' value", func_name, "param_len");
}

void expect_not_called_bt_hci_cmd_create(void)
{
	const char *func_name = "bt_hci_cmd_create";

	zassert_equal(bt_hci_cmd_create_fake.call_count, 0, "'%s()' was called unexpectedly",
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
