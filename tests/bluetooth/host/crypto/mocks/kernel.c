/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include "mocks/kernel.h"

DEFINE_FAKE_VALUE_FUNC(int64_t, k_uptime_ticks);
