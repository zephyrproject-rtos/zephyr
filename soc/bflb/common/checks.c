/*
 * Copyright (c) 2026 MASSDRIVER EI (massdriver.space)
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * Check basic configuration facts
 */

#include <zephyr/kernel.h>
#include <zephyr/devicetree.h>

/* Ensure we are setup to use the required drivers */
#if defined(CONFIG_CLOCK_CONTROL) || defined(CONFIG_OTP) || defined(CONFIG_CACHE_MANAGEMENT) \
	|| defined(CONFIG_FLASH)
BUILD_ASSERT(IS_ENABLED(CONFIG_CODE_DATA_RELOCATION),
	     "Bouffalolab Platforms require usage of relocation");
#endif

/* E24 Platforms need their special CPU-tight TCMs, namely for clock control, BL61x with E907
 * does not have this issue as the SRAM is coupled to CPU clock instead of bus clock
 */
#if (defined(CONFIG_SOC_SERIES_BL60X) || defined(CONFIG_SOC_SERIES_BL70X) \
	|| defined(CONFIG_SOC_SERIES_BL70XL)) && defined(CONFIG_CODE_DATA_RELOCATION)
BUILD_ASSERT(DT_HAS_CHOSEN(zephyr_itcm),
	     "BFLB E24 Platforms requires usage of dedicated CPU TCM relocation");
#endif
