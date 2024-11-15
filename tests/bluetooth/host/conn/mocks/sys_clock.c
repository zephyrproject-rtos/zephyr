/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "sys_clock.h"

DEFINE_FAKE_VALUE_FUNC(k_timepoint_t, sys_timepoint_calc, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(k_timeout_t, sys_timepoint_timeout, k_timepoint_t);
