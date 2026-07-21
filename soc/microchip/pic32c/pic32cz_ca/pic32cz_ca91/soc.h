/*
 * Copyright (c) 2025 Microchip Technology Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CZ_CA91_SOC_H_
#define SOC_MICROCHIP_PIC32CZ_CA91_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CZ2051CA91100)
#include <pic32cz2051ca91100.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA91144)
#include <pic32cz2051ca91144.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA91176)
#include <pic32cz2051ca91176.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA91100)
#include <pic32cz4010ca91100.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA91144)
#include <pic32cz4010ca91144.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA91176)
#include <pic32cz4010ca91176.h>
#elif defined(CONFIG_SOC_PIC32CZ4010CA91208)
#include <pic32cz4010ca91208.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

#endif /* SOC_MICROCHIP_PIC32CZ_CA91_SOC_H_ */
