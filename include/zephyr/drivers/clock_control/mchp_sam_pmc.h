/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Power Management Controller (PMC) clock helpers for Microchip SAM devices.
 * @ingroup clock_control_mchp_sam_pmc
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/microchip_sam_pmc.h>

/**
 * @defgroup clock_control_mchp_sam_pmc Microchip SAM PMC
 * @ingroup clock_control_mchp
 * @{
 */

/** @brief Slow Clock Controller (SCKC) configuration. */
struct sam_sckc_config {
	uint32_t crystal_osc: 1; /**< Use the external crystal oscillator instead of the RC. */
	uint32_t reserved: 31;   /**< Reserved. */
};

/** @brief PMC clock configuration for a single peripheral clock. */
struct sam_clk_cfg {
	uint32_t clock_type; /**< Clock type (see microchip_sam_pmc.h DT bindings). */
	uint32_t clock_id;   /**< Peripheral identifier within the PMC. */
};

/** @brief PMC device constant configuration parameters. */
struct sam_pmc_cfg {
	uint32_t *const reg;                       /**< PMC register base address. */
	const struct device *td_slck;              /**< Timing-domain slow clock device. */
	const struct device *md_slck;              /**< Main-domain slow clock device. */
	const struct device *main_xtal;            /**< Main crystal oscillator device. */
	const struct sam_sckc_config td_slck_cfg;  /**< Timing-domain slow clock configuration. */
	const struct sam_sckc_config md_slck_cfg;  /**< Main-domain slow clock configuration. */
};

/** @brief PMC device run-time data. */
struct sam_pmc_data {
	struct pmc_data *pmc; /**< Backend PMC instance data. */
};

/**
 * @brief Initialize a @ref sam_clk_cfg from a clock specifier.
 *
 * @param clock Index of the clock specifier within the node's @c clocks property.
 * @param node_id Devicetree node identifier owning the @c clocks property.
 */
#define SAM_DT_CLOCK_PMC_CFG(clock, node_id) {						\
		.clock_type = DT_CLOCKS_CELL_BY_IDX(node_id,				\
						    clock,				\
						    clock_type),			\
		.clock_id = DT_CLOCKS_CELL_BY_IDX(node_id,				\
						  clock,				\
						  peripheral_id)			\
	}

/**
 * @brief Equivalent to SAM_DT_CLOCK_PMC_CFG() for the first clock of a DT instance.
 *
 * @param inst DT instance number.
 */
#define SAM_DT_INST_CLOCK_PMC_CFG(inst) SAM_DT_CLOCK_PMC_CFG(0, DT_DRV_INST(inst))

/**
 * @brief Initialize an array of @ref sam_clk_cfg for all clocks of a node.
 *
 * @param node_id Devicetree node identifier owning the @c clocks property.
 */
#define SAM_DT_CLOCKS_PMC_CFG(node_id) {						\
		LISTIFY(DT_NUM_CLOCKS(node_id),						\
		SAM_DT_CLOCK_PMC_CFG, (,), node_id)					\
	}

/**
 * @brief Equivalent to SAM_DT_CLOCKS_PMC_CFG() for a DT instance.
 *
 * @param inst DT instance number.
 */
#define SAM_DT_INST_CLOCKS_PMC_CFG(inst) SAM_DT_CLOCKS_PMC_CFG(DT_DRV_INST(inst))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_ */
