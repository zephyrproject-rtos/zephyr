/*
 * Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_TIMER_HEXAGON_TIMER_H_
#define ZEPHYR_DRIVERS_TIMER_HEXAGON_TIMER_H_

#include <zephyr/types.h>

/*
 * Timer virtual interrupt number.  Under H2, vmtimerop(hvmt_deltatimeout)
 * causes the hypervisor to post virtual interrupt H2K_TIME_GUESTINT
 * to the guest's cpuint bitmap.  GSR.CAUSE will contain this value
 * when the interrupt event is delivered.
 */
#define HEXAGON_TIMER_IRQ          CONFIG_HEXAGON_TIMER_IRQ
#define HEXAGON_TIMER_IRQ_PRIORITY CONFIG_HEXAGON_TIMER_PRIORITY

/* Timer runtime data */
struct hexagon_timer_data {
	uint64_t accumulated_cycles;
	uint32_t cycles_per_tick;
};

#endif /* ZEPHYR_DRIVERS_TIMER_HEXAGON_TIMER_H_ */
