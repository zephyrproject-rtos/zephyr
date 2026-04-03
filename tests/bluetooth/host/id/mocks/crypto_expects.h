/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_rand() is called
 *
 *  Expected behaviour:
 *   - bt_rand() to be called once with correct parameters
 */
void expect_single_call_bt_rand(void *buf, size_t len);

/*
 *  Validate expected behaviour when bt_rand() isn't called
 *
 *  Expected behaviour:
 *   - bt_rand() isn't called at all
 */
void expect_not_called_bt_rand(void);
