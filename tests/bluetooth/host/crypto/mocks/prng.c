/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/prng.h"

DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_crypto_init);
DEFINE_FAKE_VALUE_FUNC(psa_status_t, psa_generate_random, uint8_t *, size_t);
