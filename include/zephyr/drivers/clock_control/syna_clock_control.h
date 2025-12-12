/*
 * Copyright (c) 2025 Synaptics, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SYNA_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SYNA_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/syna_clock.h>

struct clock_control_syna_config {
	mem_addr_t regs;
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SYNA_CLOCK_CONTROL_H_ */
