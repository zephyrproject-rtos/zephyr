/*
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Power Management Controller (PMC) clock helpers for Atmel SAM devices.
 * @ingroup clock_control_atmel_sam_pmc
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/atmel_sam_pmc.h>

/**
 * @defgroup clock_control_atmel_sam_pmc Atmel SAM PMC
 * @ingroup clock_control_interface_ext
 * @{
 */

/** @brief Device pointer to the PMC clock controller. */
#define SAM_DT_PMC_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(pmc))

/** @brief PMC clock configuration for a single peripheral clock. */
struct atmel_sam_pmc_config {
	uint32_t clock_type;    /**< Clock type (see atmel_sam_pmc.h DT bindings). */
	uint32_t peripheral_id; /**< Peripheral identifier within the PMC. */
};

/**
 * @brief Initialize an @ref atmel_sam_pmc_config from a clock specifier.
 *
 * @param clock_id Index of the clock specifier within the node's @c clocks property.
 * @param node_id Devicetree node identifier owning the @c clocks property.
 */
#define SAM_DT_CLOCK_PMC_CFG(clock_id, node_id)					\
	{									\
	.clock_type = DT_CLOCKS_CELL_BY_IDX(node_id, clock_id, clock_type),	\
	.peripheral_id = DT_CLOCKS_CELL_BY_IDX(node_id, clock_id, peripheral_id)\
	}

/**
 * @brief Equivalent to SAM_DT_CLOCK_PMC_CFG() for the first clock of a DT instance.
 *
 * @param inst DT instance number.
 */
#define SAM_DT_INST_CLOCK_PMC_CFG(inst) SAM_DT_CLOCK_PMC_CFG(0, DT_DRV_INST(inst))

/**
 * @brief Initialize an array of @ref atmel_sam_pmc_config for all clocks of a node.
 *
 * @param node_id Devicetree node identifier owning the @c clocks property.
 */
#define SAM_DT_CLOCKS_PMC_CFG(node_id)						\
	{									\
		LISTIFY(DT_NUM_CLOCKS(node_id),					\
			SAM_DT_CLOCK_PMC_CFG, (,), node_id)			\
	}

/**
 * @brief Equivalent to SAM_DT_CLOCKS_PMC_CFG() for a DT instance.
 *
 * @param inst DT instance number.
 */
#define SAM_DT_INST_CLOCKS_PMC_CFG(inst)					\
	SAM_DT_CLOCKS_PMC_CFG(DT_DRV_INST(inst))

/** @} */

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_ */
