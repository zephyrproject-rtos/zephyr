/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_addr_le_create_static() is called
 *
 *  Expected behaviour:
 *   - bt_addr_le_create_static() to be called once with correct parameters
 */
void expect_call_count_bt_addr_le_create_static(int call_count);

/*
 *  Validate expected behaviour when bt_addr_le_create_static() isn't called
 *
 *  Expected behaviour:
 *   - bt_addr_le_create_static() isn't called at all
 */
void expect_not_called_bt_addr_le_create_static(void);
