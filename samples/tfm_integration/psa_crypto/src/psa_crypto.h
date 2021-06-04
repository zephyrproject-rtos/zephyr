/*
 * Copyright (c) 2019,2020 Linaro Limited
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>

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

#ifdef __cplusplus
}
#endif
