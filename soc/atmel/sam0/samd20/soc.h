/*
 * Copyright (c) 2018 Sean Nyekjaer
 * Copyright (c) 2023 Ionut Catalin Pavel <iocapa@iocapa.com>
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ATMEL_SAM0_SAMD20_SOC_H_
#define _SOC_ATMEL_SAM0_SAMD20_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAMD20E14)
#include <samd20e14.h>
#elif defined(CONFIG_SOC_SAMD20E15)
#include <samd20e15.h>
#elif defined(CONFIG_SOC_SAMD20E16)
#include <samd20e16.h>
#elif defined(CONFIG_SOC_SAMD20E17)
#include <samd20e17.h>
#elif defined(CONFIG_SOC_SAMD20E18)
#include <samd20e18.h>
#elif defined(CONFIG_SOC_SAMD20G14)
#include <samd20g14.h>
#elif defined(CONFIG_SOC_SAMD20G15)
#include <samd20g15.h>
#elif defined(CONFIG_SOC_SAMD20G16)
#include <samd20g16.h>
#elif defined(CONFIG_SOC_SAMD20G17)
#include <samd20g17.h>
#elif defined(CONFIG_SOC_SAMD20G18)
#include <samd20g18.h>
#elif defined(CONFIG_SOC_SAMD20G17U)
#include <samd20g17u.h>
#elif defined(CONFIG_SOC_SAMD20G18U)
#include <samd20g18u.h>
#elif defined(CONFIG_SOC_SAMD20J14)
#include <samd20j14.h>
#elif defined(CONFIG_SOC_SAMD20J15)
#include <samd20j15.h>
#elif defined(CONFIG_SOC_SAMD20J16)
#include <samd20j16.h>
#elif defined(CONFIG_SOC_SAMD20J17)
#include <samd20j17.h>
#elif defined(CONFIG_SOC_SAMD20J18)
#include <samd20j18.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include "adc_fixup_sam0.h"
#include "../common/soc_port.h"
#include "../common/atmel_sam0_dt.h"

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ ATMEL_SAM0_DT_CPU_CLK_FREQ_HZ

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ SOC_ATMEL_SAM0_HCLK_FREQ_HZ

/** Known values */
#define SOC_ATMEL_SAM0_DFLL48M_MAX_FREQ_HZ	48000000
#define SOC_ATMEL_SAM0_OSC32K_FREQ_HZ		32768
#define SOC_ATMEL_SAM0_XOSC32K_FREQ_HZ		32768
#define SOC_ATMEL_SAM0_OSC8M_FREQ_HZ		8000000
#define SOC_ATMEL_SAM0_OSCULP32K_FREQ_HZ	32768
#define SOC_ATMEL_SAM0_GCLK1_TARGET_FREQ_HZ	31250

/** GCLK1 source frequency selector */
#if defined(CONFIG_SOC_ATMEL_SAMD_DEFAULT_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ SOC_ATMEL_SAM0_GCLK1_TARGET_FREQ_HZ
#elif defined(CONFIG_SOC_ATMEL_SAMD_OSC32K_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ SOC_ATMEL_SAM0_OSC32K_FREQ_HZ
#elif defined(CONFIG_SOC_ATMEL_SAMD_XOSC32K_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ SOC_ATMEL_SAM0_XOSC32K_FREQ_HZ
#elif defined(CONFIG_SOC_ATMEL_SAMD_OSC8M_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ SOC_ATMEL_SAM0_OSC8M_FREQ_HZ
#elif defined(CONFIG_SOC_ATMEL_SAMD_XOSC_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ CONFIG_SOC_ATMEL_SAMD_XOSC_FREQ_HZ
#else
#error Unsupported GCLK1 clock source.
#endif

/** Dividers and frequency for GCLK0 */
#define SOC_ATMEL_SAM0_GCLK0_DIV	\
	(SOC_ATMEL_SAM0_DFLL48M_MAX_FREQ_HZ / SOC_ATMEL_SAM0_MCK_FREQ_HZ)
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ

/** DFLL48M output frequency */
#define SOC_ATMEL_SAM0_DFLL48M_FREQ_HZ	\
	(SOC_ATMEL_SAM0_MCK_FREQ_HZ * SOC_ATMEL_SAM0_GCLK0_DIV)

/** Dividers and frequency for GCLK1 */
#define SOC_ATMEL_SAM0_GCLK1_DIV	\
	(SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ / SOC_ATMEL_SAM0_GCLK1_TARGET_FREQ_HZ)
#define SOC_ATMEL_SAM0_GCLK1_FREQ_HZ	\
	(SOC_ATMEL_SAM0_GCLK1_SRC_FREQ_HZ / SOC_ATMEL_SAM0_GCLK1_DIV)

/** DFLL48M output multiplier */
#define SOC_ATMEL_SAM0_DFLL48M_MUL	\
	(SOC_ATMEL_SAM0_DFLL48M_FREQ_HZ / SOC_ATMEL_SAM0_GCLK1_FREQ_HZ)

/** Frequency for GCLK2 */
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ SOC_ATMEL_SAM0_OSCULP32K_FREQ_HZ

/** Dividers and frequency for GCLK3 */
#define SOC_ATMEL_SAM0_GCLK3_FREQ_HZ SOC_ATMEL_SAM0_OSC8M_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK3_DIV	\
	(SOC_ATMEL_SAM0_DFLL48M_FREQ_HZ / SOC_ATMEL_SAM0_GCLK3_FREQ_HZ)

#define SOC_ATMEL_SAM0_APBA_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_APBB_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_APBC_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ

#endif /* _SOC_ATMEL_SAM0_SAMD20_SOC_H_ */
