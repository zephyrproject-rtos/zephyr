/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_

/**
 * @brief Enable/disable oscillators or other clocks
 */
#define NUMAKER_SCC_CLKSW_UNTOUCHED 0
#define NUMAKER_SCC_CLKSW_ENABLE    1
#define NUMAKER_SCC_CLKSW_DISABLE   2

/**
 * @brief SCC subsystem ID
 */
#define NUMAKER_SCC_SUBSYS_ID_PCC 1

struct numaker_scc_subsys {
	uint32_t subsys_id; /* SCC sybsystem ID */

	/* Peripheral clock control configuration structure
	 * clk_modidx is same as u32ModuleIdx in BSP CLK_SetModuleClock().
	 * clk_src is same as u32ClkSrc in BSP CLK_SetModuleClock().
	 * clk_div is same as u32ClkDiv in BSP CLK_SetModuleClock().
	 */
	union {
		struct {
			uint32_t clk_modidx;
			uint32_t clk_src;
			uint32_t clk_div;
		} pcc;
	};
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_ */
