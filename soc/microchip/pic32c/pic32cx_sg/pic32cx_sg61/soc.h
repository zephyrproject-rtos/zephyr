/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CX_SG61_H_
#define SOC_MICROCHIP_PIC32CX_SG61_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG61100)
#include <pic32cx1025sg61100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG61128)
#include <pic32cx1025sg61128.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CX_SG61_H_ */
