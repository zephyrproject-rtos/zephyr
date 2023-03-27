/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 * Copyright (c) 2023 Codecoup
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef MOCKS_KERNEL_H_
#define MOCKS_KERNEL_H_

#include <zephyr/fff.h>
#include <zephyr/kernel.h>

void mock_kernel_init(void);
void mock_kernel_cleanup(void);

DECLARE_FAKE_VALUE_FUNC(k_ticks_t, z_timeout_remaining, const struct _timeout *);
DECLARE_FAKE_VALUE_FUNC(int, k_work_schedule, struct k_work_delayable *, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(bool, k_work_cancel_delayable_sync, struct k_work_delayable *,
			struct k_work_sync *);

#endif /* MOCKS_KERNEL_H_ */
