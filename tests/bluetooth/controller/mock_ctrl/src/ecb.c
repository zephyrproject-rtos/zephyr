/*
 * Copyright (c) 2021 Demant
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/types.h>
#include <zephyr/ztest.h>
#include "hal/ecb.h"

__attribute__((weak)) void ecb_encrypt(uint8_t const *const key_le,
				       uint8_t const *const clear_text_le,
				       uint8_t *const cipher_text_le, uint8_t *const cipher_text_be)
{
}
