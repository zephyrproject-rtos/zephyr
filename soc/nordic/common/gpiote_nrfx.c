/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 * SPDX-License-Identifier: Apache-2.0
 */

/* Keep peripheral addresses as in real HW so we can compare them with DT values */
#define NRF_H_NO_BSIM_REDEFS

#include <zephyr/kernel.h>
#include <nrfx_gpiote.h>
#include "gpiote_nrfx.h"

#define GPIOTE_NRFX_INST_DEF(instname, reg) \
	nrfx_gpiote_t instname = NRFX_GPIOTE_INSTANCE(reg);
#define GPIOTE_NRFX_INST_DEFINE(node_id) \
	GPIOTE_NRFX_INST_DEF(GPIOTE_NRFX_INST_BY_NODE(node_id), DT_REG_ADDR(node_id))

DT_FOREACH_STATUS_OKAY(nordic_nrf_gpiote, GPIOTE_NRFX_INST_DEFINE)
