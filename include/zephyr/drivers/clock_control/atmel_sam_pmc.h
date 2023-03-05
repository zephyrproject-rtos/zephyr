/*
 * Copyright (c) 2023 Gerson Fernando Budke <nandojve@gmail.com
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_
#define ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_

#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/atmel_sam_pmc.h>

#define SAM_DT_PMC_CONTROLLER DEVICE_DT_GET(DT_NODELABEL(pmc))

struct atmel_sam_pmc_config {
	uint32_t clock_type;
	uint32_t peripheral_id;
};

#define SAM_DT_CLOCK_PMC_CFG(clock_id, node_id)					\
	{									\
	.clock_type = DT_CLOCKS_CELL_BY_IDX(node_id, clock_id, clock_type),	\
	.peripheral_id = DT_CLOCKS_CELL_BY_IDX(node_id, clock_id, peripheral_id)\
	}

#define SAM_DT_INST_CLOCK_PMC_CFG(inst) SAM_DT_CLOCK_PMC_CFG(0, DT_DRV_INST(inst))

#define SAM_DT_CLOCKS_PMC_CFG(node_id)						\
	{									\
		LISTIFY(DT_NUM_CLOCKS(node_id),					\
			SAM_DT_CLOCK_PMC_CFG, (,), node_id)			\
	}

#define SAM_DT_INST_CLOCKS_PMC_CFG(inst)					\
	SAM_DT_CLOCKS_PMC_CFG(DT_DRV_INST(inst))

#endif /* ZEPHYR_INCLUDE_DRIVERS_CLOCK_CONTROL_ATMEL_SAM_PMC_H_ */
