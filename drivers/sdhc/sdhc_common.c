/*
 * SPDX-FileCopyrightText: Copyright 2026 NXP
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/logging/log.h>

#include "sdhc_common.h"

LOG_MODULE_REGISTER(sdhc_common, CONFIG_SDHC_LOG_LEVEL);

void sdhc_common_dt_props_init(struct sdhc_host_props *props, const struct sdhc_common_config *cfg)
{
	props->max_current_330 = cfg->max_current_330;
	props->max_current_300 = cfg->max_current_300;
	props->max_current_180 = cfg->max_current_180;

	props->f_min = cfg->min_bus_freq;
	props->f_max = cfg->max_bus_freq;

	props->power_delay = cfg->power_delay_ms;

	props->bus_4_bit_support = (cfg->bus_width >= 4);

	props->hs200_support = cfg->mmc_hs200_1_8v;
	props->hs400_support = cfg->mmc_hs400_1_8v;
	props->hs400_enhanced_strobe_support = cfg->mmc_hs400_enhanced_strobe;

	LOG_DBG("SDHC properties:\n"
		"  max_current_330: %u\n"
		"  max_current_300: %u\n"
		"  max_current_180: %u\n"
		"  f_min: %u\n"
		"  f_max: %u\n"
		"  power_delay: %u\n"
		"  bus_4_bit_support: %s\n"
		"  hs200_support: %s\n"
		"  hs400_support: %s\n"
		"  hs400_enhanced_strobe_support: %s\n",
		props->max_current_330, props->max_current_300, props->max_current_180,
		props->f_min, props->f_max, props->power_delay,
		props->bus_4_bit_support ? "true" : "false",
		props->hs200_support ? "true" : "false",
		props->hs400_support ? "true" : "false",
		props->hs400_enhanced_strobe_support ? "true" : "false");
}
