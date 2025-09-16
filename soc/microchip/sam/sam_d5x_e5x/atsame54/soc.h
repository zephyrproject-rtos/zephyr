/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_SAM_D5X_E5X_ATSAME54_SOC_H_
#define SOC_MICROCHIP_SAM_D5X_E5X_ATSAME54_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_ATSAME54N19A)
#include <same54n19a.h>
#elif defined(CONFIG_SOC_ATSAME54N20A)
#include <same54n20a.h>
#elif defined(CONFIG_SOC_ATSAME54P19A)
#include <same54p19a.h>
#elif defined(CONFIG_SOC_ATSAME54P20A)
#include <same54p20a.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_SAM_D5X_E5X_ATSAME54_SOC_H_ */
