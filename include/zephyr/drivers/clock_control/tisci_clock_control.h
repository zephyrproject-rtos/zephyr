/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_

#include <stdint.h>

/**
 * @struct tisci_clock_config
 * @brief Clock configuration structure
 *
 * This structure is used to define the configuration for a clock, including
 * the device ID and clock ID.
 *
 * @param tisci_clock_config::dev_id
 * Device ID associated with the clock.
 *
 * @param tisci_clock_config::clk_id
 * Clock ID within the device.
 */
struct tisci_clock_config {
	uint32_t dev_id;
	uint32_t clk_id;
};

#define TISCI_GET_CLOCK(node_id) DEVICE_DT_GET(DT_PHANDLE(node_id, clocks))

#define TISCI_GET_CLOCK_DETAILS(node_id)	\
	{	\
		.dev_id = DT_CLOCKS_CELL(node_id, devid),	\
		.clk_id = DT_CLOCKS_CELL(node_id, clkid)	\
	}

#define TISCI_GET_CLOCK_BY_INST(inst) TISCI_GET_CLOCK(DT_DRV_INST(inst))

#define TISCI_GET_CLOCK_DETAILS_BY_INST(DT_DRV_INST) TISCI_GET_CLOCK_DETAILS(DT_DRV_INST(inst))

#endif
