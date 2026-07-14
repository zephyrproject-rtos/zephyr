/*
 * Copyright (C) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAMA5D2_SOC__H_
#define __SAMA5D2_SOC__H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if   defined(CONFIG_SOC_SAMA5D21)
  #include "sama5d21.h"
#elif defined(CONFIG_SOC_SAMA5D22)
  #include "sama5d22.h"
#elif defined(CONFIG_SOC_SAMA5D225CD1M)
  #include "sama5d225cd1m.h"
#elif defined(CONFIG_SOC_SAMA5D23)
  #include "sama5d23.h"
#elif defined(CONFIG_SOC_SAMA5D24)
  #include "sama5d24.h"
#elif defined(CONFIG_SOC_SAMA5D26)
  #include "sama5d26.h"
#elif defined(CONFIG_SOC_SAMA5D27)
  #include "sama5d27.h"
#elif defined(CONFIG_SOC_SAMA5D27CD1G)
  #include "sama5d27cd1g.h"
#elif defined(CONFIG_SOC_SAMA5D27CD5M)
  #include "sama5d27cd5m.h"
#elif defined(CONFIG_SOC_SAMA5D27CLD1G)
  #include "sama5d27cld1g.h"
#elif defined(CONFIG_SOC_SAMA5D27CLD2G)
  #include "sama5d27cld2g.h"
#elif defined(CONFIG_SOC_SAMA5D28)
  #include "sama5d28.h"
#elif defined(CONFIG_SOC_SAMA5D28CD1G)
  #include "sama5d28cd1g.h"
#elif defined(CONFIG_SOC_SAMA5D28CLD1G)
  #include "sama5d28cld1g.h"
#elif defined(CONFIG_SOC_SAMA5D28CLD2G)
  #include "sama5d28cld2g.h"
#elif defined(CONFIG_SOC_SAMA5D27SOM1)
  #include <sama5d27som1.h>
#elif defined(CONFIG_SOC_SAMA5D27WLSOM1)
  #include <sama5d27wlsom1.h>
#elif defined(CONFIG_SOC_SAMA5D29)
  #include "sama5d29.h"
#elif defined(CONFIG_SOC_SAMA5D29TA100)
  #include "sama5d29ta100.h"
#else
  #error Library does not support the specified device
#endif

#endif /* _ASMLANGUAGE */

#endif /* __SAMA5D2_SOC__H_ */
