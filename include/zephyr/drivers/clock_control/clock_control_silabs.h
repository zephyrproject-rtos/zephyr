/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Silicon Labs devices.
 * @ingroup clock_control_silabs
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_

#include <zephyr/drivers/clock_control.h>

/**
 * @defgroup clock_control_silabs Silicon Labs
 * @ingroup clock_control_interface_ext
 * @{
 */

#if defined(CONFIG_SOC_SILABS_XG21)
#include <zephyr/dt-bindings/clock/silabs/xg21-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG22)
#include <zephyr/dt-bindings/clock/silabs/xg22-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG23)
#include <zephyr/dt-bindings/clock/silabs/xg23-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG24)
#include <zephyr/dt-bindings/clock/silabs/xg24-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG26)
#include <zephyr/dt-bindings/clock/silabs/xg26-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG27)
#include <zephyr/dt-bindings/clock/silabs/xg27-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG28)
#include <zephyr/dt-bindings/clock/silabs/xg28-clock.h>
#elif defined(CONFIG_SOC_SILABS_XG29)
#include <zephyr/dt-bindings/clock/silabs/xg29-clock.h>
#endif

/** @brief Clock Management Unit (CMU) clock configuration for a peripheral. */
struct silabs_clock_control_cmu_config {
	uint32_t bus_clock; /**< Bus clock enable identifier. */
	uint8_t branch;     /**< Clock branch the peripheral is sourced from. */
};

/**
 * @brief Initialize a @ref silabs_clock_control_cmu_config from a DT node.
 *
 * @param node_id Devicetree node identifier with a @c clocks property.
 */
#define SILABS_DT_CLOCK_CFG(node_id)                                                               \
	{                                                                                          \
		.bus_clock = DT_CLOCKS_CELL(node_id, enable),                                      \
		.branch = DT_CLOCKS_CELL(node_id, branch),                                         \
	}

/**
 * @brief Equivalent to SILABS_DT_CLOCK_CFG() for a DT instance.
 *
 * @param inst DT instance number.
 */
#define SILABS_DT_INST_CLOCK_CFG(inst)                                                             \
	{                                                                                          \
		.bus_clock = DT_INST_CLOCKS_CELL(inst, enable),                                    \
		.branch = DT_INST_CLOCKS_CELL(inst, branch),                                       \
	}

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_ */
