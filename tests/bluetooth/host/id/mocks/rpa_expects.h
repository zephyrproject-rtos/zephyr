/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when bt_rpa_create() is called
 *
 *  Expected behaviour:
 *   - bt_rpa_create() to be called once with correct parameters
 */
void expect_single_call_bt_rpa_create(const uint8_t irk[16]);

/*
 *  Validate expected behaviour when bt_rpa_create() isn't called
 *
 *  Expected behaviour:
 *   - bt_rpa_create() isn't called at all
 */
void expect_not_called_bt_rpa_create(void);
