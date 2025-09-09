/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CX_SG41_H_
#define SOC_MICROCHIP_PIC32CX_SG41_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG41064)
#include <pic32cx1025sg41064.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG41080)
#include <pic32cx1025sg41080.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG41100)
#include <pic32cx1025sg41100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG41128)
#include <pic32cx1025sg41128.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CX_SG41_H_ */
