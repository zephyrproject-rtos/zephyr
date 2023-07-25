/*
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM E70 MCU.
 *
 * This file provides register access macros for the Atmel SAM E70 MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAME70_SOC_H_
#define _ATMEL_SAME70_SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE


#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAME70J19
#include <same70j19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J20
#include <same70j20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J21
#include <same70j21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N19
#include <same70n19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N20
#include <same70n20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N21
#include <same70n21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q19
#include <same70q19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q20
#include <same70q20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q21
#include <same70q21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J19B
#include <same70j19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J20B
#include <same70j20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70J21B
#include <same70j21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N19B
#include <same70n19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N20B
#include <same70n20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70N21B
#include <same70n21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q19B
#include <same70q19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q20B
#include <same70q20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAME70Q21B
#include <same70q21b.h>
#else
  #error Library does not support the specified device.
#endif

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"
#include "../common/atmel_sam_dt.h"

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ ATMEL_SAM_DT_CPU_CLK_FREQ_HZ

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ \
	(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAME70_MDIV)

/** UTMI PLL clock (UPLLCK) Frequency */
#define SOC_ATMEL_SAM_UPLLCK_FREQ_HZ MHZ(480)

#endif /* _ASMLANGUAGE */

#endif /* _ATMEL_SAME70_SOC_H_ */
