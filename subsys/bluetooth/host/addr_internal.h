/** @file
 * @brief Internal header for Bluetooth address functions
 */

/*
 * Copyright (c) 2023 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/bluetooth/addr.h>

void bt_addr_le_copy_resolved(bt_addr_le_t *dst, const bt_addr_le_t *src);

bool bt_addr_le_is_resolved(const bt_addr_le_t *addr);
