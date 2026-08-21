/*
 * Copyright (c) 2026 Demant A/S
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ESMF_TRAFFIC_LIGHT_TRAFFIC_EVENTS_H_
#define ESMF_TRAFFIC_LIGHT_TRAFFIC_EVENTS_H_

#include <zephyr/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

enum traffic_trigger {
	TRIG_TIMER = 1,
	TRIG_PED_BUTTON,
	TRIG_EMERGENCY,
	TRIG_DIAG_TICK,
	TRIG_RESET,
	TRIG_TICK,
};

int traffic_post_event(uint32_t trigger, const char *name, k_timeout_t timeout);

#ifdef __cplusplus
}
#endif

#endif /* ESMF_TRAFFIC_LIGHT_TRAFFIC_EVENTS_H_ */
