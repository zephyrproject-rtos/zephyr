/*
 * Copyright (c) 2024 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Header containing timer specific declarations for the
 * Zephyr OS layer of the Wi-Fi driver.
 */
#ifndef __TIMER_H__
#define __TIMER_H__

struct timer_list {
	void (*function)(unsigned long data);
	unsigned long data;
	struct k_work_delayable work;
};

void init_timer(struct timer_list *timer);

void mod_timer(struct timer_list *timer, int msec);

void del_timer_sync(struct timer_list *timer);

#endif /* __TIMER_H__ */
