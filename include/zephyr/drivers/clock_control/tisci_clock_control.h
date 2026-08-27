/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for the TI TISCI (System Controller) interface.
 * @ingroup clock_control_tisci
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_

#include <stdint.h>

/**
 * @defgroup clock_control_tisci Texas Instruments TISCI
 * @ingroup clock_control_interface_ext
 * @{
 */

/**
 * @brief Clock configuration structure.
 *
 * Defines the configuration for a clock, including the device ID and clock ID.
 */
struct tisci_clock_config {
	uint32_t dev_id; /**< Device ID associated with the clock. */
	uint32_t clk_id; /**< Clock ID within the device. */
};

/**
 * @brief Get the clock controller device referenced by a node's @c clocks phandle.
 *
 * @param node_id Devicetree node identifier.
 */
#define TISCI_GET_CLOCK(node_id) DEVICE_DT_GET(DT_PHANDLE(node_id, clocks))

/**
 * @brief Initialize a @ref tisci_clock_config from a node's clock specifier.
 *
 * @param node_id Devicetree node identifier.
 */
#define TISCI_GET_CLOCK_DETAILS(node_id)	\
	{	\
		.dev_id = DT_CLOCKS_CELL(node_id, devid),	\
		.clk_id = DT_CLOCKS_CELL(node_id, clkid)	\
	}

/**
 * @brief Equivalent to TISCI_GET_CLOCK() for a DT instance.
 *
 * @param inst DT instance number.
 */
#define TISCI_GET_CLOCK_BY_INST(inst) TISCI_GET_CLOCK(DT_DRV_INST(inst))

/**
 * @brief Equivalent to TISCI_GET_CLOCK_DETAILS() for a DT instance.
 *
 * @param inst DT instance number.
 */
#define TISCI_GET_CLOCK_DETAILS_BY_INST(inst) TISCI_GET_CLOCK_DETAILS(DT_DRV_INST(inst))

/** @} */

#endif
