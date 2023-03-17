/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>

#include "mock_kernel.h"

DEFINE_FAKE_VALUE_FUNC(k_ticks_t, z_timeout_remaining, const struct _timeout *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_schedule, struct k_work_delayable *, k_timeout_t);
DEFINE_FAKE_VALUE_FUNC(bool, k_work_cancel_delayable_sync, struct k_work_delayable *,
		       struct k_work_sync *);
DEFINE_FAKE_VOID_FUNC(k_work_init_delayable, struct k_work_delayable *, k_work_handler_t);
DEFINE_FAKE_VALUE_FUNC(int, k_work_cancel_delayable, struct k_work_delayable *);
DEFINE_FAKE_VALUE_FUNC(int, k_work_reschedule, struct k_work_delayable *, k_timeout_t);
