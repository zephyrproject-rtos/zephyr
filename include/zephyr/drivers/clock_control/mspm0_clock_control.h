/*
 * Copyright (c) 2025 Texas Instruments Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Texas Instruments MSPM0 devices.
 * @ingroup clock_control_mspm0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL

#include <zephyr/dt-bindings/clock/mspm0_clock.h>

/**
 * @defgroup clock_control_mspm0 Texas Instruments MSPM0
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief MSPM0 system clock subsystem selector. */
struct mspm0_sys_clock {
	uint32_t clk; /**< Clock identifier (see mspm0_clock.h DT bindings). */
};

/**
 * @brief Initialize a @ref mspm0_sys_clock from a DT instance clock specifier.
 *
 * @param index DT instance number.
 */
#define MSPM0_CLOCK_SUBSYS_FN(index) {.clk = DT_INST_CLOCKS_CELL(index, clk)}

/** @brief clock_control_configure() source selector for the MSPM0 clock controller. */
enum mspm0_clock_source {
	MSPM0_CLOCK_SRC_SYSOSC,   /**< Internal system oscillator. */
	MSPM0_CLOCK_SRC_LFCLK,    /**< Low-frequency clock. */
	MSPM0_CLOCK_SRC_LFXT,     /**< Low-frequency crystal. */
	MSPM0_CLOCK_SRC_LFOSC,    /**< Internal low-frequency oscillator. */
	MSPM0_CLOCK_SRC_LFCLK_IN, /**< External low-frequency digital input. */
	MSPM0_CLOCK_SRC_HFXT,     /**< High-frequency crystal. */
	MSPM0_CLOCK_SRC_HFCLK_IN, /**< External high-frequency digital input. */
	MSPM0_CLOCK_SRC_HFCLK,    /**< High-frequency clock. */
	MSPM0_CLOCK_SRC_HSCLK,    /**< High-speed clock. */
	MSPM0_CLOCK_SRC_SYSPLL,   /**< System PLL. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MSPM0_CLOCK_CONTROL */
