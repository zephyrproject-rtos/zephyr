/*
 * Copyright (c) 2019 Gerson Fernando Budke
 * Copyright (c) 2016 Piotr Mienkowski
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM V70 MCU.
 *
 * This file provides register access macros for the Atmel SAM V70 MCU, HAL
 * drivers for core peripherals as well as symbols specific to Atmel SAM family.
 */

#ifndef _ATMEL_SAMV70_SOC_H_
#define _ATMEL_SAMV70_SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE


#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined CONFIG_SOC_PART_NUMBER_SAMV70J19
#include <samv70j19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70J20
#include <samv70j20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70N19
#include <samv70n19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70N20
#include <samv70n20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70Q19
#include <samv70q19.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70Q20
#include <samv70q20.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70J19B
#include <samv70j19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70J20B
#include <samv70j20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70N19B
#include <samv70n19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70N20B
#include <samv70n20b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70Q19B
#include <samv70q19b.h>
#elif defined CONFIG_SOC_PART_NUMBER_SAMV70Q20B
#include <samv70q20b.h>
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
	(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAMV70_MDIV)

/** UTMI PLL clock (UPLLCK) Frequency */
#define SOC_ATMEL_SAM_UPLLCK_FREQ_HZ MHZ(480)

#endif /* _ASMLANGUAGE */

#include "pwm_fixup.h"

#endif /* _ATMEL_SAMV70_SOC_H_ */
