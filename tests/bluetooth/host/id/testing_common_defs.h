/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** Bluetooth device "test" addresses */
#define BT_ADDR ((bt_addr_t[]){{{0x0A, 0x89, 0x67, 0x45, 0x23, 0x01}}})

/** Bluetooth LE device "test" public addresses */
#define BT_LE_ADDR ((bt_addr_le_t[]){{BT_ADDR_LE_PUBLIC, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0x01}}}})
