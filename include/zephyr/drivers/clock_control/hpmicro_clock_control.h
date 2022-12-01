/*
 * Copyright (c) 2022 HPMicro
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HPMICRO_CLOCK_CONTROL_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HPMICRO_CLOCK_CONTROL_H_

#include <stdint.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/hpmicro-clocks.h>

/**
 * @brief Generate clock setting preset
 *
 */
#define HPM_MAKE_SYSCTL_PRESENT(x)		BIT(x)

/**
 * @brief hpmicro clock_control_subsys_t configure data
 *
 */

struct hpm_clock_configure_data {
	uint32_t clock_name;
	uint32_t clock_src;
	uint16_t clock_div;
};

/** Common clock control device node for all hpmicro chips */
#define HPMICRO_CLOCK_CONTROL_NODE DT_NODELABEL(clk)

#define HPM_CLOCK_CFG_DATA_DEFAULT(declar, n) \
		.declar.clock_name = DT_INST_CLOCKS_CELL(n, name),   \
		.declar.clock_src = DT_INST_CLOCKS_CELL(n, src),   \
		.declar.clock_div = DT_INST_CLOCKS_CELL(n, div),

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_HPMICRO_CLOCK_CONTROL_H_ */
