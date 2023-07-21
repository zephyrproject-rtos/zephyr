/*
 * Copyright (c) 2023 Graphcore Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <zephyr/fff.h>
#include <zephyr/ztest.h>
#include <zephyr/kernel.h>

DECLARE_FAKE_VALUE_FUNC(int, k_mutex_lock, struct k_mutex*, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(int, k_mutex_unlock, struct k_mutex*);
