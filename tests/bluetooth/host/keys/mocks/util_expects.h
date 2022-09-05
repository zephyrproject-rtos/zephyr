/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when u8_to_dec() is called
 *
 *  Expected behaviour:
 *   - u8_to_dec() to be called once with correct parameters
 */
void expect_single_call_u8_to_dec(uint8_t value);

/*
 *  Validate expected behaviour when u8_to_dec() isn't called
 *
 *  Expected behaviour:
 *   - u8_to_dec() isn't called at all
 */
void expect_not_called_u8_to_dec(void);
