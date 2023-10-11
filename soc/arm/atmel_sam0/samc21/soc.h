/*
 * Copyright (c) 2022 Kamil Serwus
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _ATMEL_SAMC_SOC_H_
#define _ATMEL_SAMC_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>


#if defined(CONFIG_SOC_PART_NUMBER_SAMC21E15A)
#include <samc21e15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21E16A)
#include <samc21e16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21E17A)
#include <samc21e17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21E18A)
#include <samc21e18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21G15A)
#include <samc21g15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21G16A)
#include <samc21g16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21G17A)
#include <samc21g17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21G18A)
#include <samc21g18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J15A)
#include <samc21j15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J16A)
#include <samc21j16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J17A)
#include <samc21j17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J17AU)
#include <samc21j17au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J18A)
#include <samc21j18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21J18AU)
#include <samc21j18au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21N17A)
#include <samc21n17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC21N18A)
#include <samc21n18a.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include "adc_fixup_sam0.h"
#include "../common/soc_port.h"
#include "../common/atmel_sam0_dt.h"

#define SOC_ATMEL_SAM0_OSC32K_FREQ_HZ 32768

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ SOC_ATMEL_SAM0_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ

#endif /* _ATMEL_SAMD51_SOC_H_ */
