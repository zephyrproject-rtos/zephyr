/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_SAM_D5X_E5X_ATSAMD51_SOC_H_
#define SOC_MICROCHIP_SAM_D5X_E5X_ATSAMD51_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_ATSAMD51G18A)
#include <samd51g18a.h>
#elif defined(CONFIG_SOC_ATSAMD51G19A)
#include <samd51g19a.h>
#elif defined(CONFIG_SOC_ATSAMD51J18A)
#include <samd51j18a.h>
#elif defined(CONFIG_SOC_ATSAMD51J19A)
#include <samd51j19a.h>
#elif defined(CONFIG_SOC_ATSAMD51J20A)
#include <samd51j20a.h>
#elif defined(CONFIG_SOC_ATSAMD51N19A)
#include <samd51n19a.h>
#elif defined(CONFIG_SOC_ATSAMD51N20A)
#include <samd51n20a.h>
#elif defined(CONFIG_SOC_ATSAMD51P19A)
#include <samd51p19a.h>
#elif defined(CONFIG_SOC_ATSAMD51P20A)
#include <samd51p20a.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_SAM_D5X_E5X_ATSAMD51_SOC_H_ */
