/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** Bluetooth device "test" addresses */
#define BT_ADDR ((bt_addr_t[]){{{0x0A, 0x89, 0x67, 0x45, 0x23, 0x01}}})

/** Bluetooth LE public device "test" addresses */
#define BT_LE_ADDR ((bt_addr_le_t[]){{BT_ADDR_LE_PUBLIC, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0x01}}}})

/** Bluetooth LE device "test" static random addresses */
#define BT_STATIC_RANDOM_LE_ADDR_1                                                                 \
	((bt_addr_le_t[]){{BT_ADDR_LE_RANDOM, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0xC1}}}})
#define BT_STATIC_RANDOM_LE_ADDR_2                                                                 \
	((bt_addr_le_t[]){{BT_ADDR_LE_RANDOM, {{0x0B, 0x89, 0x67, 0x45, 0x23, 0xC1}}}})

/** Bluetooth LE device "test" random resolvable private addresses */
#define BT_RPA_LE_ADDR                                                                             \
	((bt_addr_le_t[]){{BT_ADDR_LE_RANDOM, {{0x0A, 0x89, 0x67, 0x45, 0x23, 0x41}}}})
