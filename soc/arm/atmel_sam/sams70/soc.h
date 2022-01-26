/*
 * Copyright (c) 2021 RIC Electronics
 * Copyright (c) 2019 Gerson Fernando Budke
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM S70 MCU.
 *
 * This file provides register access macros for the Atmel SAM S70 MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAMS70_SOC_H_
#define _ATMEL_SAMS70_SOC_H_

#include <sys/util.h>

#ifndef _ASMLANGUAGE

/* Add include for DTS generated information */
#include <devicetree.h>

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAMS70J19
#include <sams70j19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70J20
#include <sams70j20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70J21
#include <sams70j21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N19
#include <sams70n19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N20
#include <sams70n20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N21
#include <sams70n21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q19
#include <sams70q19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q20
#include <sams70q20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q21
#include <sams70q21.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70J19B
#include <sams70j19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70J20B
#include <sams70j20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70J21B
#include <sams70j21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N19B
#include <sams70n19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N20B
#include <sams70n20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70N21B
#include <sams70n21b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q19B
#include <sams70q19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q20B
#include <sams70q20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMS70Q21B
#include <sams70q21b.h>
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
	(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAMS70_MDIV)

#endif /* _ASMLANGUAGE */

#include "pwm_fixup.h"
#include "fixups.h"

#endif /* _ATMEL_SAMS70_SOC_H_ */
