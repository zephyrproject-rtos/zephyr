/*
 * Copyright (C) 2025-2026 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <pmc.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(pmc_setup, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

void sam_pmc_setup(const struct device *dev)
{
	ARG_UNUSED(dev);

	LOG_WRN("To be implemented.");
}

struct device *sam_pmc_get_clock(const struct sam_clk_cfg *cfg,
				 struct pmc_data *ptr)
{
	ARG_UNUSED(cfg);
	ARG_UNUSED(ptr);

	LOG_WRN("To be implemented.");

	return NULL;
}
