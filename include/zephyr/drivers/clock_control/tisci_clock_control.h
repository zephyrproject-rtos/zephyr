/*
 * Copyright (c) 2025 Texas Instruments
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_TISCI_CLOCK_CONTROL_H_

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
#include <stdint.h>
struct tisci_clock_config {
	uint32_t dev_id;
	uint32_t clk_id;
};

#define TISCI_GET_CLOCK(_dev) DEVICE_DT_GET(DT_PHANDLE(DT_NODELABEL(_dev), clocks))

#define TISCI_GET_CLOCK_DETAILS(_dev)                                                              \
	{.dev_id = DT_CLOCKS_CELL(DT_NODELABEL(_dev), devid),                                      \
	 .clk_id = DT_CLOCKS_CELL(DT_NODELABEL(_dev), clkid)}

#define TISCI_GET_CLOCK_BY_INST(inst) DEVICE_DT_INST_GET(inst)

#define TISCI_GET_CLOCK_DETAILS_BY_INST(inst)                                                      \
	{.dev_id = DT_INST_CLOCKS_CELL(inst, devid), .clk_id = DT_INST_CLOCKS_CELL(inst, clkid)}
#endif
