/*
 * Copyright (c) 2026 Silicon Laboratories Inc.
 * SPDX-License-Identifier: Apache-2.0
 *
 * Zephyr port of the CPC timer (see port/silabs/inc/sli_cpc_timer_types.h).
 */

#ifndef SLI_CPC_TIMER_TYPES_H
#define SLI_CPC_TIMER_TYPES_H

#include <stdbool.h>

#include <zephyr/kernel.h>

typedef struct sli_cpc_timer {
	struct k_timer timer;
	void (*callback)(struct sli_cpc_timer *timer, void *data);
	void *data;
	bool running;
} sli_cpc_timer_t;

#endif /* SLI_CPC_TIMER_TYPES_H */
