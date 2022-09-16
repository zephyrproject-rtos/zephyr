/*
 * Copyright (c) 2019-2020 Gerson Fernando Budke <nandojve@gmail.com>
 * Copyright (c) 2018 Vincent van der Locht
 * Copyright (c) 2017 Justin Watson
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the Atmel SAM4E family processors.
 */

#ifndef _ATMEL_SAM4E_SOC_H_
#define _ATMEL_SAM4E_SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE


#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined(CONFIG_SOC_PART_NUMBER_SAM4E16E)
#include <sam4e16e.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4E16C)
#include <sam4e16c.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4E8E)
#include <sam4e8e.h>
#elif defined(CONFIG_SOC_PART_NUMBER_SAM4E8C)
#include <sam4e8c.h>
#else
#error Library does not support the specified device.
#endif

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"
#include "../common/atmel_sam_dt.h"

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ ATMEL_SAM_DT_CPU_CLK_FREQ_HZ

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ SOC_ATMEL_SAM_HCLK_FREQ_HZ

#endif /* !_ASMLANGUAGE */

#endif /* _ATMEL_SAM4E_SOC_H_ */
