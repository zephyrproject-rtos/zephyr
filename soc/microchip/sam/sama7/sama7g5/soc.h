/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_SAMA7G5_SOC_H_
#define SOC_MICROCHIP_SAMA7G5_SOC_H_

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

#endif /* SOC_MICROCHIP_SAMA7G5_SOC_H_ */
