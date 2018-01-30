/*
 * Copyright (c) 2017 Google LLC.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAMD_SOC_H_
#define _ATMEL_SAMD_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PART_NUMBER_SAMD21E15A)
#include <samd21e15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21E16A)
#include <samd21e16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21E17A)
#include <samd21e17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21E18A)
#include <samd21e18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G15A)
#include <samd21g15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G16A)
#include <samd21g16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G17A)
#include <samd21g17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G17AU)
#include <samd21g17au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G18A)
#include <samd21g18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21G18AU)
#include <samd21g18au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21J15A)
#include <samd21j15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21J16A)
#include <samd21j16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21J17A)
#include <samd21j17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMD21J18A)
#include <samd21j18a.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ SOC_ATMEL_SAM0_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_XOSC32K_FREQ_HZ 32768
#define SOC_ATMEL_SAM0_OSC8M_FREQ_HZ 8000000
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ

#if defined(CONFIG_SOC_ATMEL_SAMD_XOSC32K_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_FREQ_HZ SOC_ATMEL_SAM0_XOSC32K_FREQ_HZ
#elif defined(CONFIG_SOC_ATMEL_SAMD_OSC8M_AS_MAIN)
#define SOC_ATMEL_SAM0_GCLK1_FREQ_HZ SOC_ATMEL_SAM0_OSC8M_FREQ_HZ
#else
#error Unsupported GCLK1 clock source.
#endif

#define SOC_ATMEL_SAM0_GCLK3_FREQ_HZ SOC_ATMEL_SAM0_OSC8M_FREQ_HZ
#define SOC_ATMEL_SAM0_APBA_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_APBB_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_APBC_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ

#endif /* _ATMEL_SAMD_SOC_H_ */
