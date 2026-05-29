/* long_wq.h - Workqueue API intended for long-running operations. */

/*
 * Copyright (c) 2022 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/sys_clock.h>

int bt_long_wq_schedule(struct k_work_delayable *dwork, k_timeout_t timeout);
int bt_long_wq_reschedule(struct k_work_delayable *dwork, k_timeout_t timeout);
int bt_long_wq_submit(struct k_work *dwork);
