/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/util.h"

DEFINE_FAKE_VALUE_FUNC(uint8_t, u8_to_dec, char *, uint8_t, uint8_t);
