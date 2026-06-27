/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

/**
 * @file
 * @brief Clock control header file for the Nuvoton NuMicro family.
 *
 * This file provides clock driver interface definitions and structures
 * for the Nuvoton NuMicro family.
 * @ingroup clock_control_numicro
 */

#ifndef ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_
#define ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_

/**
 * @defgroup clock_control_numicro Nuvoton NuMicro
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief SCC subsystem ID for peripheral clock control (PCC). */
#define NUMICRO_SCC_SUBSYS_ID_PCC 1

/** @brief Peripheral clock control configuration structure */
struct numicro_scc_subsys_pcc {
	/** Clock module */
	uint32_t clk_mod;
	/** Clock source */
	uint32_t clk_src;
	/** Clock divider */
	uint32_t clk_div;
};

/** @brief Numicro peripheral clock subsystem configuration */
struct numicro_scc_subsys {
	/** Subsystem ID (currently hardcoded to PCC) */
	uint32_t subsys_id;

	/** Subsystem config */
	union {
		/** Peripheral subsystem config */
		struct numicro_scc_subsys_pcc pcc;
	};
};

/** @brief Create Numicro peripheral clock subsystem configuration from device tree */
#define DT_NUMICRO_CLOCK_PCC_SUBSYSTEM(dev)                                                        \
	{                                                                                          \
		.subsys_id = NUMICRO_SCC_SUBSYS_ID_PCC,                                            \
		.pcc =                                                                             \
			{                                                                          \
				.clk_mod = DT_CLOCKS_CELL(dev, clock_module_index),                \
				.clk_src = DT_CLOCKS_CELL(dev, clock_source),                      \
				.clk_div = DT_CLOCKS_CELL(dev, clock_divider),                     \
			},                                                                         \
	}

/** @brief Create Numicro peripheral clock subsystem configuration from device tree */
#define DT_NUMICRO_CLOCK_PCC_SUBSYSTEM_INST(inst) DT_NUMICRO_CLOCK_PCC_SUBSYSTEM(DT_DRV_INST(inst))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_ */
