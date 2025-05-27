/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMA7G5_SOC__H_
#define __SAMA7G5_SOC__H_

#ifdef CONFIG_SOC_SAMA7G54
	#define __SAMA7G54__
#endif

#include "sam.h"

/* number of clocks registered */
#define SOC_NUM_CLOCK_PLL_FRAC     7
#define SOC_NUM_CLOCK_PLL_DIV      (SOC_NUM_CLOCK_PLL_FRAC + 1) /* AUDIO PLL: DIVPMC, DIVIO */
#define SOC_NUM_CLOCK_MASTER       5	/* MCK 0 ~ 4 */
#define SOC_NUM_CLOCK_PROGRAMMABLE 8	/* MCK 0 ~ 7 */
#define SOC_NUM_CLOCK_SYSTEM       8	/* PCK 0 ~ 7 */
#define SOC_NUM_CLOCK_PERIPHERAL   72
#define SOC_NUM_CLOCK_GENERATED    46

enum PLL_ID {
	PLL_ID_CPUPLL = 0,
	PLL_ID_SYSPLL = 1,
	PLL_ID_DDRPLL = 2,
	PLL_ID_IMGPLL = 3,
	PLL_ID_BAUDPLL = 4,
	PLL_ID_AUDIOPLL = 5,
	PLL_ID_ETHPLL = 6,
};

#endif /* __SAMA7G5_SOC__H_ */
