/*
 * Copyright (c) 2018 Vincent van der Locht
 * Copyright (c) 2017 Justin Watson
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM4S family processors.
 */

#ifndef _ATMEL_SAM4S_SOC_H_
#define _ATMEL_SAM4S_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined(CONFIG_SOC_PART_NUMBER_SAM4S16C)
#include <sam4s16c.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S16B)
#include <sam4s16b.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S8C)
#include <sam4s8c.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S8B)
#include <sam4s8b.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S4C)
#include <sam4s4c.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S4B)
#include <sam4s4b.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S4A)
#include <sam4s4a.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S2C)
#include <sam4s2c.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S2B)
#include <sam4s2b.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4S2A)
#include <sam4s2a.h>
#else
#error Library does not support the specified device.
#endif

#include "soc_pinmap.h"

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"

#endif /* !_ASMLANGUAGE */

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ DT_ARM_CORTEX_M4_0_CLOCK_FREQUENCY

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ SOC_ATMEL_SAM_HCLK_FREQ_HZ

#endif /* _ATMEL_SAM4S_SOC_H_ */
