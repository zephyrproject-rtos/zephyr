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


#if defined(CONFIG_SOC_PART_NUMBER_SAMC20E15A)
#include <samc20e15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20E16A)
#include <samc20e16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20E17A)
#include <samc20e17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20E18A)
#include <samc20e18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20G15A)
#include <samc20g15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20G16A)
#include <samc20g16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20G17A)
#include <samc20g17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20G18A)
#include <samc20g18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J15A)
#include <samc20j15a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J16A)
#include <samc20j16a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J17A)
#include <samc20j17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J17AU)
#include <samc20j17au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J18A)
#include <samc20j18a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20J18AU)
#include <samc20j18au.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20N17A)
#include <samc20n17a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAMC20N18A)
#include <samc20n18a.h>
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
