/*
 * Copyright (c) 2025-2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CX_SG60_H_
#define SOC_MICROCHIP_PIC32CX_SG60_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG60100)
#include <pic32cx1025sg60100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG60128)
#include <pic32cx1025sg60128.h>
#else
#error "Library does not support the specified device."
#endif

#include "pic32cx_sg.h"

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CX_SG60_H_ */
