/*
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM3X MCU.
 *
 * This file provides register access macros for the Atmel SAM3X MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAM3X_SOC_H_
#define _ATMEL_SAM3X_SOC_H_

#ifndef _ASMLANGUAGE

/* Add include for DTS generated information */
#include <devicetree.h>

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAM3X4C
#include <sam3x4c.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3X4E
#include <sam3x4e.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3X8C
#include <sam3x8c.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3X8E
#include <sam3x8e.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3X8H
#include <sam3x8h.h>
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

#endif /* _ASMLANGUAGE */

#endif /* _ATMEL_SAM3X_SOC_H_ */
