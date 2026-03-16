/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_SAMA7D6_SOC__H_
#define _SOC_MICROCHIP_SAMA7D6_SOC__H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if   defined(CONFIG_SOC_SAMA7D65)
  #include <sama7d65.h>
#elif defined(CONFIG_SOC_SAMA7D65D1G)
  #include <sama7d65d1g.h>
#elif defined(CONFIG_SOC_SAMA7D65D1GN2)
  #include <sama7d65d1gn2.h>
#elif defined(CONFIG_SOC_SAMA7D65D2G)
  #include <sama7d65d2g.h>
#elif defined(CONFIG_SOC_SAMA7D65D2GN8)
  #include <sama7d65d2gn8.h>
#elif defined(CONFIG_SOC_SAMA7D65D5M)
  #include <sama7d65d5m.h>
#else
  #error Library does not support the specified device
#endif

#endif /* _ASMLANGUAGE */

/* number of clocks registered */
#define SOC_NUM_CLOCK_PLL_FRAC     9
#define SOC_NUM_CLOCK_PLL_DIV      (SOC_NUM_CLOCK_PLL_FRAC + 1) /* AUDIO PLL: DIVPMC, DIVIO */
#define SOC_NUM_CLOCK_MASTER       10	/* MCK 0 ~ 9 */
#define SOC_NUM_CLOCK_PROGRAMMABLE 8	/* MCK 0 ~ 7 */
#define SOC_NUM_CLOCK_SYSTEM       8	/* PCK 0 ~ 7 */
#define SOC_NUM_CLOCK_PERIPHERAL   72
#define SOC_NUM_CLOCK_GENERATED    44

#endif /* _SOC_MICROCHIP_SAMA7D6_SOC__H_ */
