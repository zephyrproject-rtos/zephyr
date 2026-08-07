/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_NORDIC_COMMON_GPIOTE_NRFX_H_
#define SOC_NORDIC_COMMON_GPIOTE_NRFX_H_

#include <nrfx_gpiote.h>

#define GPIOTE_NRFX_INST_BY_REG_CONCAT(reg) g_nrfx_gpiote##reg
#define GPIOTE_NRFX_INST_BY_REG(reg) GPIOTE_NRFX_INST_BY_REG_CONCAT(reg)
#define GPIOTE_NRFX_INST_BY_NODE(node) GPIOTE_NRFX_INST_BY_REG(DT_REG_ADDR(node))

#define GPIOTE_NRFX_INST_DECL(instname) \
	extern nrfx_gpiote_t instname;
#define GPIOTE_NRFX_INST_DECLARE(node_id) \
	GPIOTE_NRFX_INST_DECL(GPIOTE_NRFX_INST_BY_NODE(node_id))

DT_FOREACH_STATUS_OKAY(nordic_nrf_gpiote, GPIOTE_NRFX_INST_DECLARE)

#endif /* SOC_NORDIC_COMMON_GPIOTE_NRFX_H_ */
