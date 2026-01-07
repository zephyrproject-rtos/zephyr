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

/* Peripheral clock control configuration structure
 *
 * clk_modidx is virtual of u32ModuleIdx/u64ModuleIdx in BSP CLK_SetModuleClock().
 * clk_src is same as u32ClkSrc in BSP CLK_SetModuleClock().
 * clk_div is same as u32ClkDiv in BSP CLK_SetModuleClock().
 */
struct numaker_scc_subsys_pcc {
	uint32_t clk_modidx;
	uint32_t clk_src;
	uint32_t clk_div;
};

/* Peripheral clock control rate structure
 *
 * clk_modidx_real is real of u32ModuleIdx/u64ModuleIdx in BSP CLK_SetModuleClock().
 * clk_src_idx is decoded clock source index of clk_src.
 * clk_src_rate is clock source rate of clk_src_idx.
 * clk_div_value is decoded clock divider value of clk_div.
 * clk_div_value_max is supported maximum divider value of clk_div_value.
 * clk_mod_rate is module rate.
 */
struct numaker_scc_subsys_pcc_rate {
#if defined(CONFIG_SOC_SERIES_M55M1X)
	uint64_t clk_modidx_real;
#else
	uint32_t clk_modidx_real;
#endif
	uint32_t clk_src_idx;
	uint32_t clk_src_rate;
	uint32_t clk_div_value;
	uint32_t clk_div_value_max;
	uint32_t clk_mod_rate;
};

struct numaker_scc_subsys {
	uint32_t subsys_id; /* SCC subsystem ID */

	union {
		struct numaker_scc_subsys_pcc pcc;
	};
};

struct numaker_scc_subsys_rate {
	union {
		struct numaker_scc_subsys_pcc_rate pcc;
	};
};

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_ */
