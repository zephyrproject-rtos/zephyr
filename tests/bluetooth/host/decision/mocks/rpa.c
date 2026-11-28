/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/rpa.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_rpa_ah, const uint8_t *, const uint8_t *, uint8_t *);
