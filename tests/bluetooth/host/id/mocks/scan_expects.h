/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_le_scan_set_enable() is called
 *
 *  Expected behaviour:
 *   - bt_le_scan_set_enable() to be called once with correct parameters
 */
void expect_call_count_bt_le_scan_set_enable(int call_count, uint8_t args_history[]);

/*
 *  Validate expected behaviour when bt_le_scan_set_enable() isn't called
 *
 *  Expected behaviour:
 *   - bt_le_scan_set_enable() isn't called at all
 */
void expect_not_called_bt_le_scan_set_enable(void);
