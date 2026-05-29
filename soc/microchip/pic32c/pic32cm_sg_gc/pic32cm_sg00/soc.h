/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CM_SG00_H_
#define SOC_MICROCHIP_PIC32CM_SG00_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CM5112SG00048)
#include "pic32cm5112sg00048.h"
#elif defined(CONFIG_SOC_PIC32CM5112SG00064)
#include "pic32cm5112sg00064.h"
#elif defined(CONFIG_SOC_PIC32CM5112SG00100)
#include "pic32cm5112sg00100.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CM_SG00_H_ */
