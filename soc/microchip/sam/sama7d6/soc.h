/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMA7D6_SOC__H_
#define __SAMA7D6_SOC__H_

#ifdef CONFIG_SOC_SAMA7D65
#define __SAMA7D65__
#endif

#include "sam.h"

/* number of clocks registered */
#define SOC_NUM_CLOCK_PLL_FRAC     9
#define SOC_NUM_CLOCK_PLL_DIV      (SOC_NUM_CLOCK_PLL_FRAC + 1) /* AUDIO PLL: DIVPMC, DIVIO */
#define SOC_NUM_CLOCK_MASTER       10	/* MCK 0 ~ 9 */
#define SOC_NUM_CLOCK_PROGRAMMABLE 8	/* MCK 0 ~ 7 */
#define SOC_NUM_CLOCK_SYSTEM       8	/* PCK 0 ~ 7 */
#define SOC_NUM_CLOCK_PERIPHERAL   72
#define SOC_NUM_CLOCK_GENERATED    44

#endif /* __SAMA7D6_SOC__H_ */
