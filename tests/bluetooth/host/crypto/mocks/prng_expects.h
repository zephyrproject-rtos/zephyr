/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

/*
 *  Validate expected behaviour when psa_crypto_init() is called
 *
 *  Expected behaviour:
 *   - psa_crypto_init() to be called once with correct parameters
 */
void expect_single_call_tc_psa_crypto_init(void);

/*
 *  Validate expected behaviour when psa_generate_random() is called
 *
 *  Expected behaviour:
 *   - psa_generate_random() to be called once with correct parameters
 */
void expect_single_call_psa_generate_random(uint8_t *out, unsigned int outlen);
