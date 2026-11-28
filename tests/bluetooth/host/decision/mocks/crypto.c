/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/crypto.h"

DEFINE_FAKE_VALUE_FUNC(int, bt_rand, void *, size_t);
