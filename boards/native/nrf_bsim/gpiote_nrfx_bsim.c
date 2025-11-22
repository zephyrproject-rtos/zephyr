/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <nrfx_gpiote.h>
#include "gpiote_nrfx.h"

#define GPIOTE_NRFX_INST_IDX(idx) DT_CAT3(NRFX_GPIOTE, idx, _INST_IDX)
#define GPIOTE_INST_IDX(node_id) GPIOTE_NRFX_INST_IDX(DT_PROP(node_id, instance))
#define GPIOTE_INST_AND_COMMA(node_id) \
	[GPIOTE_INST_IDX(node_id)] = &GPIOTE_NRFX_INST_BY_NODE(node_id),

static int gpiote_bsim_init(void)
{
	nrfx_gpiote_t *gpiote_instances[] = {
		DT_FOREACH_STATUS_OKAY(nordic_nrf_gpiote, GPIOTE_INST_AND_COMMA)
	};

	/* For simulated devices we need to convert the hardcoded DT address from the real
	 * peripheral into the correct one for simulation
	 */
	for (int inst = 0; inst < ARRAY_SIZE(gpiote_instances); inst++) {
		gpiote_instances[inst]->p_reg =
			nhw_convert_periph_base_addr(gpiote_instances[inst]->p_reg);
	}

	return 0;
}

SYS_INIT(gpiote_bsim_init, PRE_KERNEL_1, 0);
