/** @file
 * @brief Modem workqueue header file.
 */

/*
 * Copyright (c) 2025 Embeint Pty Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_
#define ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifdef CONFIG_MODEM_DEDICATED_WORKQUEUE

int modem_work_submit(struct k_work *work);
int modem_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay);
int modem_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay);

#else

static inline int modem_work_submit(struct k_work *work)
{
	return k_work_submit(work);
}

static inline int modem_work_schedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_schedule(dwork, delay);
}

static inline int modem_work_reschedule(struct k_work_delayable *dwork, k_timeout_t delay)
{
	return k_work_reschedule(dwork, delay);
}

#endif /* CONFIG_MODEM_DEDICATED_WORKQUEUE */

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_MODEM_WORKQUEUE_H_ */
