/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_keys_find_irk() is called
 *
 *  Expected behaviour:
 *   - bt_keys_find_irk() to be called once with correct parameters
 */
void expect_single_call_bt_keys_find_irk(uint8_t id, const bt_addr_le_t *addr);

/*
 *  Validate expected behaviour when bt_keys_find_irk() isn't called
 *
 *  Expected behaviour:
 *   - bt_keys_find_irk() isn't called at all
 */
void expect_not_called_bt_keys_find_irk(void);

/*
 *  Validate expected behaviour when bt_keys_foreach_type() is called
 *
 *  Expected behaviour:
 *   - bt_keys_foreach_type() to be called once with correct parameters
 */
void expect_single_call_bt_keys_foreach_type(enum bt_keys_type type);

/*
 *  Validate expected behaviour when bt_keys_foreach_type() isn't called
 *
 *  Expected behaviour:
 *   - bt_keys_foreach_type() isn't called at all
 */
void expect_not_called_bt_keys_foreach_type(void);
