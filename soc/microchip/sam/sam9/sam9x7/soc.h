/*
 * Copyright (C) 2025 Microchip Technology Inc. and its subsidiaries
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef __SAM9X7_SOC__H_
#define __SAM9X7_SOC__H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAM9X70)
  #include <sam9x70.h>
#elif defined(CONFIG_SOC_SAM9X72)
  #include <sam9x72.h>
#elif defined(CONFIG_SOC_SAM9X75)
  #include <sam9x75.h>
#elif defined(CONFIG_SOC_SAM9X75D1G)
  #include <sam9x75d1gn0.h>
#elif defined(CONFIG_SOC_SAM9X75D1GN2)
  #include <sam9x75d1gn2.h>
#elif defined(CONFIG_SOC_SAM9X75D2G)
  #include <sam9x75d2g.h>
#elif defined(CONFIG_SOC_SAM9X75D2GN4)
  #include <sam9x75d2gn4.h>
#elif defined(CONFIG_SOC_SAM9X75D5M)
  #include <sam9x75d5m.h>
#elif defined(CONFIG_SOC_SAM9X75D5MN0)
  #include <sam9x75d5mn0.h>
#else
  #error Library does not support the specified device
#endif

#endif /* _ASMLANGUAGE */

#endif /* __SAM9X7_SOC__H_ */
