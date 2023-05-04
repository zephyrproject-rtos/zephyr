/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/rpa.h"

#include <zephyr/kernel.h>

DEFINE_FAKE_VALUE_FUNC(int, bt_rpa_create, const uint8_t *, bt_addr_t *);
