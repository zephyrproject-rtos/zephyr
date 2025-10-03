/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CM_JH01_SOC_H_
#define SOC_MICROCHIP_PIC32CM_JH01_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CM2532JH01032)
#include <pic32cm2532jh01032.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH01032)
#include <pic32cm5164jh01032.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH01048)
#include <pic32cm2532jh01048.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH01048)
#include <pic32cm5164jh01048.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH01064)
#include <pic32cm2532jh01064.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH01064)
#include <pic32cm5164jh01064.h>
#elif defined(CONFIG_SOC_PIC32CM2532JH01100)
#include <pic32cm2532jh01100.h>
#elif defined(CONFIG_SOC_PIC32CM5164JH01100)
#include <pic32cm5164jh01100.h>
#elif defined(CONFIG_SOC_PIC32CM1216JH01032)
#include <pic32cm1216jh01032.h>
#elif defined(CONFIG_SOC_PIC32CM1216JH01048)
#include <pic32cm1216jh01048.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CM_JH01_SOC_H_ */
