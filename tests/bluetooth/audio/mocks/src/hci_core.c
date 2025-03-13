/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#include <zephyr/bluetooth/addr.h>
#include <zephyr/fff.h>

bool bt_le_bond_exists(uint8_t id, const bt_addr_le_t *addr)
{
	return true;
}
