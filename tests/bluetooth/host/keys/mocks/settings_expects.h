/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_settings_encode_key() is called
 *
 *  Expected behaviour:
 *   - bt_settings_encode_key() to be called once with correct parameters
 *   - 'Keys' argument isn't NULL
 */
void expect_single_call_bt_settings_encode_key_with_not_null_key(const bt_addr_le_t *addr);

/*
 *  Validate expected behaviour when bt_settings_encode_key() is called
 *
 *  Expected behaviour:
 *   - bt_settings_encode_key() to be called once with correct parameters
 *   - 'Keys' argument is NULL
 */
void expect_single_call_bt_settings_encode_key_with_null_key(const bt_addr_le_t *addr);

/*
 *  Validate expected behaviour when bt_settings_encode_key() isn't called
 *
 *  Expected behaviour:
 *   - bt_settings_encode_key() isn't called at all
 */
void expect_not_called_bt_settings_encode_key(void);
