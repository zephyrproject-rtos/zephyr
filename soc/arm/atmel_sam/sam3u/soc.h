/*
 * Copyright (c) 2023 Chen Xingyu <hi@xingrz.me>
 * Copyright (c) 2016 Intel Corporation.
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM3U MCU.
 *
 * This file provides register access macros for the Atmel SAM3U MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAM3U_SOC_H_
#define _ATMEL_SAM3U_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#if defined CONFIG_SOC_PART_NUMBER_SAM3U1C
#include <sam3u1c.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3U1E
#include <sam3u1e.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3U2C
#include <sam3u2c.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3U2E
#include <sam3u2e.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3U4C
#include <sam3u4c.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAM3U4E
#include <sam3u4e.h>
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

#endif /* _ATMEL_SAM3U_SOC_H_ */
