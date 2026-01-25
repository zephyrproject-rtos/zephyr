/*
 * Copyright (c) 2026 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CM_PL10_SOC_H_
#define SOC_MICROCHIP_PIC32CM_PL10_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CM6408PL10028)
#include <pic32cm6408pl10028.h>
#elif defined(CONFIG_SOC_PIC32CM6408PL10032)
#include <pic32cm6408pl10032.h>
#elif defined(CONFIG_SOC_PIC32CM6408PL10048)
#include <pic32cm6408pl10048.h>
#elif defined(CONFIG_SOC_PIC32CM6408PL10064)
#include <pic32cm6408pl10064.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CM_PL10_SOC_H_ */
