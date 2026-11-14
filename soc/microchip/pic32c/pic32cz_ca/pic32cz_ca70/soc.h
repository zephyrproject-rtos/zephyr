/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef SOC_MICROCHIP_PIC32CZ_CA70_SOC_H_
#define SOC_MICROCHIP_PIC32CZ_CA70_SOC_H_

#ifndef _ASMLANGUAGE

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CZ2051CA70064)
#include <pic32cz2051ca70064.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA70100)
#include <pic32cz2051ca70100.h>
#elif defined(CONFIG_SOC_PIC32CZ2051CA70144)
#include <pic32cz2051ca70144.h>
#else
#error "Library does not support the specified device."
#endif

#endif /* _ASMLANGUAGE */

/* Clock counts used by the PMC clock framework (soc/microchip/sam/common). */
#define SOC_NUM_CLOCK_PLL_FRAC     1
#define SOC_NUM_CLOCK_PLL_DIV      1
#define SOC_NUM_CLOCK_MASTER       2
#define SOC_NUM_CLOCK_PROGRAMMABLE 8
#define SOC_NUM_CLOCK_SYSTEM       8
#define SOC_NUM_CLOCK_PERIPHERAL   74
#define SOC_NUM_CLOCK_GENERATED    74

#endif /* SOC_MICROCHIP_PIC32CZ_CA70_SOC_H_ */
