/*
 * Copyright (c) 2024 Croxel, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/bluetooth/uuid.h>

/** @brief UUID of BT Service (Compatible with NUS).
 * Service: 6e400001-b5a3-f393-e0a9-e50e24dcca9e
 * RX Char: 6e400002-b5a3-f393-e0a9-e50e24dcca9e
 * TX Char: 6e400003-b5a3-f393-e0a9-e50e24dcca9e
 */
#define BT_UUID_SHELL_SRV_VAL \
	BT_UUID_128_ENCODE(0x6e400001, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_SHELL_RX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400002, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)
#define BT_UUID_SHELL_TX_CHAR_VAL \
	BT_UUID_128_ENCODE(0x6e400003, 0xb5a3, 0xf393, 0xe0a9, 0xe50e24dcca9e)

/**
 * @brief This function provides pointer to the shell BT backend instance.
 *
 * Function returns pointer to the shell BT instance. This instance can be
 * next used with shell_execute_cmd function in order to test commands behavior.
 *
 * @returns Pointer to the shell instance.
 */
const struct shell *shell_backend_bt_get_ptr(void);
