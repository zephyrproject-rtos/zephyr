/*
 * Copyright (c) 2025 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL

#include <zephyr/dt-bindings/clock/mspm0_clock.h>

struct mspm0_sys_clock {
	uint32_t clk;
};

#define MSPM0_CLOCK_SUBSYS_FN(index) {.clk = DT_INST_CLOCKS_CELL(index, clk)}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL */
