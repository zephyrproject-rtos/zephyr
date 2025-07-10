/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_SAM_D5X_E5X_ATSAME51_SOC_H_
#define SOC_MICROCHIP_SAM_D5X_E5X_ATSAME51_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_ATSAME51G18A)
#include <same51j18a.h>
#elif defined(CONFIG_SOC_ATSAME51G19A)
#include <same51j19a.h>
#elif defined(CONFIG_SOC_ATSAME51J18A)
#include <same51j18a.h>
#elif defined(CONFIG_SOC_ATSAME51J19A)
#include <same51j19a.h>
#elif defined(CONFIG_SOC_ATSAME51J20A)
#include <same51j20a.h>
#elif defined(CONFIG_SOC_ATSAME51N19A)
#include <same51n19a.h>
#elif defined(CONFIG_SOC_ATSAME51N20A)
#include <same51n20a.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_SAM_D5X_E5X_ATSAME51_SOC_H_ */
