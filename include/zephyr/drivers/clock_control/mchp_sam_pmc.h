/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_

#include <soc.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/microchip_sam_pmc.h>

struct sam_sckc_config {
	uint32_t crystal_osc: 1;
	uint32_t reserved: 31;
};

struct sam_clk_cfg {
	uint32_t clock_type;
	uint32_t clock_id;
};

/* Device constant configuration parameters */
struct sam_pmc_cfg {
	uint32_t *const reg;
	const struct device *td_slck;
	const struct device *md_slck;
	const struct device *main_xtal;
	const struct sam_sckc_config td_slck_cfg;
	const struct sam_sckc_config md_slck_cfg;
};

/* Device run time data */
struct sam_pmc_data {
	struct pmc_data *pmc;
};

#define SAM_DT_CLOCK_PMC_CFG(clock, node_id) {						\
		.clock_type = DT_CLOCKS_CELL_BY_IDX(node_id,				\
						    clock,				\
						    clock_type),			\
		.clock_id = DT_CLOCKS_CELL_BY_IDX(node_id,				\
						  clock,				\
						  peripheral_id)			\
	}

#define SAM_DT_INST_CLOCK_PMC_CFG(inst) SAM_DT_CLOCK_PMC_CFG(0, DT_DRV_INST(inst))

#define SAM_DT_CLOCKS_PMC_CFG(node_id) {						\
		LISTIFY(DT_NUM_CLOCKS(node_id),						\
		SAM_DT_CLOCK_PMC_CFG, (,), node_id)					\
	}

#define SAM_DT_INST_CLOCKS_PMC_CFG(inst) SAM_DT_CLOCKS_PMC_CFG(DT_DRV_INST(inst))

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_MICROCHIP_SAM_PMC_H_ */
