/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include "mocks/kernel.h"

DEFINE_FAKE_VALUE_FUNC(int, k_mutex_lock, struct k_mutex *, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(int, k_mutex_unlock, struct k_mutex *);
