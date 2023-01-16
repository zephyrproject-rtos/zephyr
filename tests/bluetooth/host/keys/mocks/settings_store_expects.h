/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when settings_delete() is called
 *
 *  Expected behaviour:
 *   - settings_delete() to be called once with correct parameters
 */
void expect_single_call_settings_delete(void);
