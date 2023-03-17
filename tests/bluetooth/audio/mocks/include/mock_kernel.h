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

/* List of fakes used by this unit tester */
#define KERNEL_FFF_FAKES_LIST(FAKE)                                                                \
	FAKE(z_timeout_remaining)                                                                  \
	FAKE(k_work_schedule)                                                                      \
	FAKE(k_work_cancel_delayable_sync)                                                         \
	FAKE(k_work_init_delayable)                                                                \
	FAKE(k_work_cancel_delayable)                                                              \
	FAKE(k_work_reschedule)                                                                    \

DECLARE_FAKE_VALUE_FUNC(k_ticks_t, z_timeout_remaining, const struct _timeout *);
DECLARE_FAKE_VALUE_FUNC(int, k_work_schedule, struct k_work_delayable *, k_timeout_t);
DECLARE_FAKE_VALUE_FUNC(bool, k_work_cancel_delayable_sync, struct k_work_delayable *,
			struct k_work_sync *);
DECLARE_FAKE_VOID_FUNC(k_work_init_delayable, struct k_work_delayable *, k_work_handler_t);
DECLARE_FAKE_VALUE_FUNC(int, k_work_cancel_delayable, struct k_work_delayable *);
DECLARE_FAKE_VALUE_FUNC(int, k_work_reschedule, struct k_work_delayable *, k_timeout_t);

#endif /* MOCKS_KERNEL_H_ */
