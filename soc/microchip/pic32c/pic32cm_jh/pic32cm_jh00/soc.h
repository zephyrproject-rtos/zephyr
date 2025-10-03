/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CM_JH00_SOC_H_
#define SOC_MICROCHIP_PIC32CM_JH00_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CM2532JH00032)
#include <pic32cm2532jh00032.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH00032)
#include <pic32cm5164jh00032.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH00048)
#include <pic32cm2532jh00048.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH00048)
#include <pic32cm5164jh00048.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH00064)
#include <pic32cm2532jh00064.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH00064)
#include <pic32cm5164jh00064.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH00100)
#include <pic32cm2532jh00100.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH00100)
#include <pic32cm5164jh00100.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CM_JH00_SOC_H_ */
