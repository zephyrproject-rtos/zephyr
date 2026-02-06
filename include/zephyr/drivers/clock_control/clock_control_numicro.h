/*
 * Copyright (c) 2026 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <stdint.h>

/**
 * @file clock_control_numicro.h
 * @brief Clock control header file for Nuvoton Numicro family.
 *
 * This file provides clock driver interface definitions and structures
 * fur Nuvoton Numicro family.
 */

#ifndef ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_
#define ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_

/**
 * @brief SCC Subsystem ID
 */
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

#endif /* ZEPHYR_INCLUDE_DRIVER_CLOCK_CONTROL_NUMICRO_H_ */
