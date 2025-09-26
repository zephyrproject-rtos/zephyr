/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CZ_CA80_SOC_H_
#define SOC_MICROCHIP_PIC32CZ_CA80_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CZ2051CA80100)
#include <pic32cz2051ca80100.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA80144)
#include <pic32cz2051ca80144.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA80176)
#include <pic32cz2051ca80176.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA80208)
#include <pic32cz2051ca80208.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA80100)
#include <pic32cz4010ca80100.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA80144)
#include <pic32cz4010ca80144.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA80176)
#include <pic32cz4010ca80176.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA80208)
#include <pic32cz4010ca80208.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA80100)
#include <pic32cz8110ca80100.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA80144)
#include <pic32cz8110ca80144.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA80176)
#include <pic32cz8110ca80176.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA80208)
#include <pic32cz8110ca80208.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CZ_CA80_SOC_H_ */
