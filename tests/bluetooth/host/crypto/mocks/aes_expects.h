/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when psa_cipher_encrypt() is called
 *
 *  Expected behaviour:
 *   - psa_cipher_encrypt() to be called once with correct parameters
 */
void expect_single_call_psa_cipher_encrypt(uint8_t *out);
