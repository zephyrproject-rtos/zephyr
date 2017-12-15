/*
 * Copyright (c) 2016 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for Nordic Semiconductor nRF52 family processor
 *
 * This module provides routines that rely on kernel services
 * (POST_KERNEL) to initialize and support board-level hardware for
 * the Nordic Semiconductor nRF52 family processor.
 */

#include <init.h>
#include "nrf.h"

static void check_build_soc_matches_runtime_soc(void);

static int nordicsemi_nrf52_init_post_kernel(struct device *arg)
{
	ARG_UNUSED(arg);

	check_build_soc_matches_runtime_soc();

	return 0;
}

#if !(defined(CONFIG_SOC_NRF52832) || defined(CONFIG_SOC_NRF52840))
#error "An SoC was not specified"
/* When more SoCs are added the check_build_soc_matches_runtime_soc
 * function needs to be updated
 */
#endif

static void check_build_soc_matches_runtime_soc(void)
{
	u32_t soc = IS_ENABLED(CONFIG_SOC_NRF52832) ? 0x52832UL : 0x52840UL;

	if (NRF_FICR->INFO.PART != soc) {
		/* Wrong BOARD.
		 *
		 * The FW has been built for an SoC that does not match the
		 * detected SoC, to prevent hard-to-debug undefined behaviour
		 * we panic.
		 */

		k_panic();
	}
}

SYS_INIT(nordicsemi_nrf52_init_post_kernel, POST_KERNEL, 0);
