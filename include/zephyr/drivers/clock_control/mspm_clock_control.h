/*
 * Copyright (c) 2025 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Texas Instruments MSPM devices.
 * @ingroup clock_control_mspm
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM_CLOCK_CONTROL
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM_CLOCK_CONTROL

#include <zephyr/dt-bindings/clock/mspm_clock.h>

/**
 * @defgroup clock_control_mspm Texas Instruments MSPM
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief MSPM system clock subsystem selector. */
struct mspm_sys_clock {
	uint32_t clk; /**< Clock identifier (see mspm_clock.h DT bindings). */
};

/**
 * @brief Initialize a @ref mspm_sys_clock from a DT instance clock specifier.
 *
 * @param index DT instance number.
 */
#define MSPM_CLOCK_SUBSYS_FN(index) {.clk = DT_INST_CLOCKS_CELL(index, clk)}

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM_CLOCK_CONTROL */
