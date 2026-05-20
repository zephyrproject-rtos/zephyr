/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CM_GC00_H_
#define SOC_MICROCHIP_PIC32CM_GC00_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CM5112GC00048)
#include "pic32cm5112gc00048.h"
#elif defined(CONFIG_SOC_PIC32CM5112GC00064)
#include "pic32cm5112gc00064.h"
#elif defined(CONFIG_SOC_PIC32CM5112GC00100)
#include "pic32cm5112gc00100.h"
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CM_GC00_H_ */
