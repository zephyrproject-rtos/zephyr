/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMA7G5_SOC__H_
#define __SAMA7G5_SOC__H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAMA7G54)
  #include <sama7g54.h>
#elif defined(CONFIG_SOC_SAMA7G54D1G)
  #include <sama7g54d1g.h>
#elif defined(CONFIG_SOC_SAMA7G54D1GN0)
  #include <sama7g54d1gn0.h>
#elif defined(CONFIG_SOC_SAMA7G54D1GN2)
  #include <sama7g54d1gn2.h>
#elif defined(CONFIG_SOC_SAMA7G54D2G)
  #include <sama7g54d2g.h>
#elif defined(CONFIG_SOC_SAMA7G54D2GN4)
  #include <sama7g54d2gn4.h>
#elif defined(CONFIG_SOC_SAMA7G54D4G)
  #include <sama7g54d4g.h>
#else
  #error Library does not support the specified device
#endif

#endif /* _ASMLANGUAGE */

/* number of clocks registered */
#define SOC_NUM_CLOCK_PLL_FRAC     7
#define SOC_NUM_CLOCK_PLL_DIV      (SOC_NUM_CLOCK_PLL_FRAC + 1) /* AUDIO PLL: DIVPMC, DIVIO */
#define SOC_NUM_CLOCK_MASTER       5	/* MCK 0 ~ 4 */
#define SOC_NUM_CLOCK_PROGRAMMABLE 8	/* MCK 0 ~ 7 */
#define SOC_NUM_CLOCK_SYSTEM       8	/* PCK 0 ~ 7 */
#define SOC_NUM_CLOCK_PERIPHERAL   72
#define SOC_NUM_CLOCK_GENERATED    46

#endif /* __SAMA7G5_SOC__H_ */
