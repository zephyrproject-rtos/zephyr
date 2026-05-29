/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>
#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc_data, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

static struct pmc_data pmc_clocks;

struct device *sam_pmc_get_clock(const struct sam_clk_cfg *cfg,
				 struct pmc_data *ptr)
{
	uint32_t idx = cfg->clock_id;
	uint32_t type = cfg->clock_type;

	switch (type) {
	case PMC_TYPE_CORE:
		if (idx < ptr->ncore) {
			return ptr->chws[idx];
		}
		break;
	case PMC_TYPE_SYSTEM:
		if (idx < ptr->nsystem) {
			return ptr->shws[idx];
		}
		break;
	case PMC_TYPE_PERIPHERAL:
		if (idx < ptr->nperiph) {
			return ptr->phws[idx];
		}
		break;
	case PMC_TYPE_GCK:
		if (idx < ptr->ngck) {
			return ptr->ghws[idx];
		}
		break;
	case PMC_TYPE_PROGRAMMABLE:
		if (idx < ptr->npck) {
			return ptr->pchws[idx];
		}
		break;
	default:
		break;
	}

	return NULL;
}

struct pmc_data *pmc_data_allocate(unsigned int ncore, unsigned int nsystem,
				   unsigned int nperiph, unsigned int ngck,
				   unsigned int npck, struct device **table)
{
	struct pmc_data *ptr = &pmc_clocks;

	ptr->ncore = ncore;
	ptr->chws = table;

	ptr->nsystem = nsystem;
	ptr->shws = ptr->chws + ncore;

	ptr->nperiph = nperiph;
	ptr->phws = ptr->shws + nsystem;

	ptr->ngck = ngck;
	ptr->ghws = ptr->phws + nperiph;

	ptr->npck = npck;
	ptr->pchws = ptr->ghws + ngck;

	return ptr;
}
