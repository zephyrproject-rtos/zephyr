/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_INTERNAL_H_
#define ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_INTERNAL_H_

#include <zephyr/task_wdt/task_wdt.h>

/* Lock protecting the channels and the supervised entities */
k_spinlock_key_t task_wdt_lock(void);
void task_wdt_unlock(k_spinlock_key_t key);

/*
 * Report a fault of a supervised entity on its channel, unless it is no longer attached to it.
 * Must be called without holding the lock.
 */
void task_wdt_channel_fault(struct task_wdt_supervision *sup, int channel_id,
			    enum task_wdt_fault_reason reason);

/*
 * The functions below must be called with the lock held.
 */

/* Attach a supervised entity to a channel, returns -EINVAL or -EBUSY on failure */
int task_wdt_channel_sup_attach(int channel_id, struct task_wdt_supervision *sup);

/* Detach the supervised entity from its channel and cancel its deadline */
void task_wdt_channel_sup_detach(int channel_id);

/* Set the deadline of a channel in absolute ticks, INT64_MAX to cancel it */
void task_wdt_channel_deadline_set(int channel_id, int64_t deadline_abs_ticks);

#endif /* ZEPHYR_SUBSYS_TASK_WDT_TASK_WDT_INTERNAL_H_ */
