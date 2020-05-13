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
 * @brief Demonstrates how to calcute the SHA-256 hash of a value in chunks.
 */
void crp_test_sha256(void);

/**
 * @brief Runs a series of PSA Cryptography API test functions.
 */
void crp_test(void);

#ifdef __cplusplus
}
#endif
