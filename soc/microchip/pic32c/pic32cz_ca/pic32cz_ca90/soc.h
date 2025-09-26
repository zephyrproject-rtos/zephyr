/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CZ_CA90_SOC_H_
#define SOC_MICROCHIP_PIC32CZ_CA90_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CZ2051CA90100)
#include <pic32cz2051ca90100.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA90144)
#include <pic32cz2051ca90144.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA90176)
#include <pic32cz2051ca90176.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA90208)
#include <pic32cz2051ca90208.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA90100)
#include <pic32cz4010ca90100.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA90144)
#include <pic32cz4010ca90144.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA90176)
#include <pic32cz4010ca90176.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA90208)
#include <pic32cz4010ca90208.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA90100)
#include <pic32cz8110ca90100.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA90144)
#include <pic32cz8110ca90144.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA90176)
#include <pic32cz8110ca90176.h>
#elif defined(CONFIG_SOC_PIC32CZ8110CA90208)
#include <pic32cz8110ca90208.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CZ_CA90_SOC_H_ */
