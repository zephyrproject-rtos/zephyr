/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <errno.h>
#include <zephyr/init.h>
#include <psa/crypto.h>

static int _psa_crypto_init(void)
{
	if (psa_crypto_init() != PSA_SUCCESS) {
		return -EIO;
	}

	return 0;
}

/* Enforcing initialization of PSA crypto before any other users
 * like entropy_psa_crypto (which has a higher priority number).
 * This is done without dependency on CONFIG_MBEDTLS_INIT.
 */
SYS_INIT(_psa_crypto_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
