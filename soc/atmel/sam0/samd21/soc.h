/*
 * Copyright (c) 2017 Google LLC.
 * Copyright (c) 2023 Ionut Catalin Pavel <iocapa@iocapa.com>
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ATMEL_SAM0_SAMD21_SOC_H_
#define _SOC_ATMEL_SAM0_SAMD21_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAMD21E15A)
#include <samd21e15a.h>
#elif defined(CONFIG_SOC_SAMD21E16A)
#include <samd21e16a.h>
#elif defined(CONFIG_SOC_SAMD21E17A)
#include <samd21e17a.h>
#elif defined(CONFIG_SOC_SAMD21E18A)
#include <samd21e18a.h>
#elif defined(CONFIG_SOC_SAMD21G15A)
#include <samd21g15a.h>
#elif defined(CONFIG_SOC_SAMD21G16A)
#include <samd21g16a.h>
#elif defined(CONFIG_SOC_SAMD21G17A)
#include <samd21g17a.h>
#elif defined(CONFIG_SOC_SAMD21G18A)
#include <samd21g18a.h>
#elif defined(CONFIG_SOC_SAMD21G17AU)
#include <samd21g17au.h>
#elif defined(CONFIG_SOC_SAMD21G18AU)
#include <samd21g18au.h>
#elif defined(CONFIG_SOC_SAMD21J15A)
#include <samd21j15a.h>
#elif defined(CONFIG_SOC_SAMD21J16A)
#include <samd21j16a.h>
#elif defined(CONFIG_SOC_SAMD21J17A)
#include <samd21j17a.h>
#elif defined(CONFIG_SOC_SAMD21J18A)
#include <samd21j18a.h>
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

#endif /* _SOC_ATMEL_SAM0_SAMD21_SOC_H_ */
