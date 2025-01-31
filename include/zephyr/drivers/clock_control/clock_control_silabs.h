/*
 * Copyright (c) 2024 Silicon Laboratories Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_

#include <zephyr/drivers/clock_control.h>

#if defined(CONFIG_SOC_SERIES_EFR32MG21)
#include <zephyr/dt-bindings/clock/silabs/xg21-clock.h>
#elif defined(CONFIG_SOC_SERIES_EFR32BG22)
#include <zephyr/dt-bindings/clock/silabs/xg22-clock.h>
#elif defined(CONFIG_SOC_SERIES_EFR32ZG23)
#include <zephyr/dt-bindings/clock/silabs/xg23-clock.h>
#elif defined(CONFIG_SOC_SERIES_EFR32MG24)
#include <zephyr/dt-bindings/clock/silabs/xg24-clock.h>
#elif defined(CONFIG_SOC_SERIES_EFR32BG27)
#include <zephyr/dt-bindings/clock/silabs/xg27-clock.h>
#elif defined(CONFIG_SOC_SERIES_EFR32BG29) || defined(CONFIG_SOC_SERIES_EFR32MG29)
#include <zephyr/dt-bindings/clock/silabs/xg29-clock.h>
#endif

struct silabs_clock_control_cmu_config {
	uint32_t bus_clock;
	uint8_t branch;
};

#define SILABS_DT_CLOCK_CFG(node_id)                                                               \
	{                                                                                          \
		.bus_clock = DT_CLOCKS_CELL(node_id, enable),                                      \
		.branch = DT_CLOCKS_CELL(node_id, branch),                                         \
	}

#define SILABS_DT_INST_CLOCK_CFG(inst)                                                             \
	{                                                                                          \
		.bus_clock = DT_INST_CLOCKS_CELL(inst, enable),                                    \
		.branch = DT_INST_CLOCKS_CELL(inst, branch),                                       \
	}

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_SILABS_H_ */
