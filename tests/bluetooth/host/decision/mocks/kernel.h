/*
 * Copyright (c) 2026 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

/* List of fakes used by this unit tester */
#define KERNEL_FFF_FAKES_LIST(FAKE)                                                                \
	FAKE(k_mutex_lock)                                                                         \
	FAKE(k_mutex_unlock)

DECLARE_FAKE_VALUE_FUNC(int, k_mutex_lock, struct k_mutex *, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(int, k_mutex_unlock, struct k_mutex *);
