/*
 * Copyright (c) 2022 Stephanos Ioannidis <root@stephanos.io>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/addr.h>

const bt_addr_t bt_addr_any = { { 0, 0, 0, 0, 0, 0 } };
const bt_addr_t bt_addr_none = { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } };
const bt_addr_le_t bt_addr_le_any = { 0, { { 0, 0, 0, 0, 0, 0 } } };
const bt_addr_le_t bt_addr_le_none = { 0, { { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff } } };

struct bt_addr_tmp_str bt_addr_tmp_str(const bt_addr_t *addr)
{
	struct bt_addr_tmp_str val;

	(void)bt_addr_to_str(addr, val.str, sizeof(val.str));

	return val;
}

struct bt_addr_le_tmp_str bt_addr_le_tmp_str(const bt_addr_le_t *addr)
{
	struct bt_addr_le_tmp_str val;

	(void)bt_addr_le_to_str(addr, val.str, sizeof(val.str));

	return val;
}
