/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/aes.h"

DEFINE_FAKE_VALUE_FUNC(int, tc_aes_encrypt, uint8_t *, const uint8_t *, const TCAesKeySched_t);
DEFINE_FAKE_VALUE_FUNC(int, tc_aes128_set_encrypt_key, TCAesKeySched_t, const uint8_t *);
