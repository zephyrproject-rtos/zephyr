/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "psa/crypto.h"
#include "psa/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Generates random values using the TF-M crypto service.
 */
void crp_test_rng(void);

/**
 * @brief Runs a series of PSA Cryptography API test functions.
 */
void crp_test(void);

/**
 * @brief Generates device certificate signing request (CSR) using Mbed TLS
 * X.509 and TF-M crypto service.
 */
void crp_generate_csr(void);

#ifdef __cplusplus
}
#endif
