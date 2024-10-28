/*
 * Copyright (c) 2024 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __TIMER_MANAGEMENT_H_
#define __TIMER_MANAGEMENT_H_

#include <zephyr/kernel.h>

#define TMR_COUNT 50

struct tmr_dynamic {
	struct k_timer timer;
	int id;
	int period;
	bool wasCreated;
	bool wasExpired;
};

void tmr_expiry_function(struct k_timer *timer_id);
void tmr_create(void);
void tmr_summary(void);

#endif
