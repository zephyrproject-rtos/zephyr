/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/fff.h>
#include <tinycrypt/aes.h>
#include <tinycrypt/constants.h>

/* List of fakes used by this unit tester */
#define AES_FFF_FAKES_LIST(FAKE)               \
		FAKE(tc_aes_encrypt)                   \
		FAKE(tc_aes128_set_encrypt_key)

DECLARE_FAKE_VALUE_FUNC(int, tc_aes_encrypt, uint8_t *, const uint8_t *, const TCAesKeySched_t);
DECLARE_FAKE_VALUE_FUNC(int, tc_aes128_set_encrypt_key, TCAesKeySched_t, const uint8_t *);
