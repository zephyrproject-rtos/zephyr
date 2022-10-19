/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when tc_aes_encrypt() is called
 *
 *  Expected behaviour:
 *   - tc_aes_encrypt() to be called once with correct parameters
 */
void expect_single_call_tc_aes_encrypt(uint8_t *out);
