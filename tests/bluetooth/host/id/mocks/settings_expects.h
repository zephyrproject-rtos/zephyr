/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_settings_store_id() is called
 *
 *  Expected behaviour:
 *   - bt_settings_store_id() to be called once with correct parameters
 */
void expect_single_call_bt_settings_store_id(void);

/*
 *  Validate expected behaviour when bt_settings_store_id() isn't called
 *
 *  Expected behaviour:
 *   - bt_settings_store_id() isn't called at all
 */
void expect_not_called_bt_settings_store_id(void);

/*
 *  Validate expected behaviour when bt_settings_store_irk() is called
 *
 *  Expected behaviour:
 *   - bt_settings_store_irk() to be called once with correct parameters
 */
void expect_single_call_bt_settings_store_irk(void);

/*
 *  Validate expected behaviour when bt_settings_store_irk) isn't called
 *
 *  Expected behaviour:
 *   - bt_settings_store_irk() isn't called at all
 */
void expect_not_called_bt_settings_store_irk(void);
