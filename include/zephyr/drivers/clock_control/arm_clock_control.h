/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock subsystem definitions for ARM CMSDK-based SoCs.
 * @ingroup clock_control_arm
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ARM_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ARM_CLOCK_CONTROL_H_

#include <zephyr/drivers/clock_control.h>

/**
 * @defgroup clock_control_arm ARM CMSDK
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief CMSDK peripheral bus type. */
enum arm_bus_type_t {
	CMSDK_AHB = 0, /**< AHB peripheral bus. */
	CMSDK_APB,     /**< APB peripheral bus. */
};

/** @brief CPU power state for which a clock can be configured. */
enum arm_soc_state_t {
	SOC_ACTIVE = 0,  /**< Active (run) state. */
	SOC_SLEEP,       /**< Sleep state. */
	SOC_DEEPSLEEP,   /**< Deep sleep state. */
};

/** @brief Clock control subsystem descriptor for ARM CMSDK SoCs. */
struct arm_clock_control_t {
	enum arm_bus_type_t bus;     /**< Peripheral bus the device sits on. */
	enum arm_soc_state_t state;  /**< CPU state the clock setting applies to. */
	uint32_t device;             /**< Identifies the device on the bus. */
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ARM_CLOCK_CONTROL_H_ */
