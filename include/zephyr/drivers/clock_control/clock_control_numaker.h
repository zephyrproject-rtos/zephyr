/*
 * Copyright (c) 2023 Nuvoton Technology Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Clock control definitions for Nuvoton NuMaker devices.
 * @ingroup clock_control_numaker
 */

#include <stdint.h>

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_

/**
 * @defgroup clock_control_numaker Nuvoton NuMaker
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @name Oscillator/clock switch actions */
/** @{ */
#define NUMAKER_SCC_CLKSW_UNTOUCHED 0 /**< Leave the clock switch unchanged. */
#define NUMAKER_SCC_CLKSW_ENABLE    1 /**< Enable the oscillator/clock. */
#define NUMAKER_SCC_CLKSW_DISABLE   2 /**< Disable the oscillator/clock. */
/** @} */

/** @brief SCC subsystem ID for peripheral clock control (PCC). */
#define NUMAKER_SCC_SUBSYS_ID_PCC 1

/** @brief Peripheral clock control configuration. */
struct numaker_scc_subsys_pcc {
	/** @brief Module index (u32ModuleIdx/u64ModuleIdx in BSP CLK_SetModuleClock()). */
	uint32_t clk_modidx;
	/** @brief Clock source (u32ClkSrc in BSP CLK_SetModuleClock()). */
	uint32_t clk_src;
	/** @brief Clock divider (u32ClkDiv in BSP CLK_SetModuleClock()). */
	uint32_t clk_div;
};

/** @brief Peripheral clock control rate information. */
struct numaker_scc_subsys_pcc_rate {
#if defined(CONFIG_SOC_SERIES_M55M1X)
	uint64_t clk_modidx_real; /**< Resolved module index. */
#else
	uint32_t clk_modidx_real; /**< Resolved module index. */
#endif
	uint32_t clk_src_idx;       /**< Decoded clock source index of @c clk_src. */
	uint32_t clk_src_rate;      /**< Clock source rate, in Hz. */
	uint32_t clk_div_value;     /**< Decoded clock divider value. */
	uint32_t clk_div_value_max; /**< Maximum supported divider value. */
	uint32_t clk_mod_rate;      /**< Resulting module clock rate, in Hz. */
};

/** @brief NuMaker clock subsystem selector. */
struct numaker_scc_subsys {
	uint32_t subsys_id; /**< SCC subsystem ID (see @ref NUMAKER_SCC_SUBSYS_ID_PCC). */

	union {
		struct numaker_scc_subsys_pcc pcc; /**< Peripheral clock configuration. */
	};
};

/** @brief NuMaker clock subsystem rate selector. */
struct numaker_scc_subsys_rate {
	union {
		struct numaker_scc_subsys_pcc_rate pcc; /**< Peripheral clock rate information. */
	};
};

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_NUMAKER_H_ */
