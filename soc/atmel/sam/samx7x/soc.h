/*
 * Copyright (c) 2016 Piotr Mienkowski
 * Copyright (c) 2019-2024 Gerson Fernando Budke <nandojve@gmail.com>
 * SPDX-License-Identifier: Apache-2.0
 */

/** @file
 * @brief Register access macros for the Atmel SAM E70/S70/V70/V71 MCU series.
 *
 * This file provides register access macros for the Atmel SAM E70/S70/V70/V71 MCU series, HAL
 * drivers for core peripherals as well as symbols specific to this Atmel SAM family.
 */

#ifndef _SOC_ATMEL_SAM_SAMX7X_SOC_H_
#define _SOC_ATMEL_SAM_SAMX7X_SOC_H_

#include <zephyr/sys/util.h>

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT
#define DONT_USE_PREDEFINED_CORE_HANDLERS
#define DONT_USE_PREDEFINED_PERIPHERALS_HANDLERS

#if defined(CONFIG_SOC_SAME70J19)
#include <same70j19.h>
#elif defined(CONFIG_SOC_SAME70J20)
#include <same70j20.h>
#elif defined(CONFIG_SOC_SAME70J21)
#include <same70j21.h>
#elif defined(CONFIG_SOC_SAME70N19)
#include <same70n19.h>
#elif defined(CONFIG_SOC_SAME70N20)
#include <same70n20.h>
#elif defined(CONFIG_SOC_SAME70N21)
#include <same70n21.h>
#elif defined(CONFIG_SOC_SAME70Q19)
#include <same70q19.h>
#elif defined(CONFIG_SOC_SAME70Q20)
#include <same70q20.h>
#elif defined(CONFIG_SOC_SAME70Q21)
#include <same70q21.h>
#elif defined(CONFIG_SOC_SAME70J19B)
#include <same70j19b.h>
#elif defined(CONFIG_SOC_SAME70J20B)
#include <same70j20b.h>
#elif defined(CONFIG_SOC_SAME70J21B)
#include <same70j21b.h>
#elif defined(CONFIG_SOC_SAME70N19B)
#include <same70n19b.h>
#elif defined(CONFIG_SOC_SAME70N20B)
#include <same70n20b.h>
#elif defined(CONFIG_SOC_SAME70N21B)
#include <same70n21b.h>
#elif defined(CONFIG_SOC_SAME70Q19B)
#include <same70q19b.h>
#elif defined(CONFIG_SOC_SAME70Q20B)
#include <same70q20b.h>
#elif defined(CONFIG_SOC_SAME70Q21B)
#include <same70q21b.h>
#elif defined(CONFIG_SOC_SAMV71J19)
#include <samv71j19.h>
#elif defined(CONFIG_SOC_SAMV71J20)
#include <samv71j20.h>
#elif defined(CONFIG_SOC_SAMV71J21)
#include <samv71j21.h>
#elif defined(CONFIG_SOC_SAMV71N19)
#include <samv71n19.h>
#elif defined(CONFIG_SOC_SAMV71N20)
#include <samv71n20.h>
#elif defined(CONFIG_SOC_SAMV71N21)
#include <samv71n21.h>
#elif defined(CONFIG_SOC_SAMV71Q19)
#include <samv71q19.h>
#elif defined(CONFIG_SOC_SAMV71Q20)
#include <samv71q20.h>
#elif defined(CONFIG_SOC_SAMV71Q21)
#include <samv71q21.h>
#elif defined(CONFIG_SOC_SAMV71J19B)
#include <samv71j19b.h>
#elif defined(CONFIG_SOC_SAMV71J20B)
#include <samv71j20b.h>
#elif defined(CONFIG_SOC_SAMV71J21B)
#include <samv71j21b.h>
#elif defined(CONFIG_SOC_SAMV71N19B)
#include <samv71n19b.h>
#elif defined(CONFIG_SOC_SAMV71N20B)
#include <samv71n20b.h>
#elif defined(CONFIG_SOC_SAMV71N21B)
#include <samv71n21b.h>
#elif defined(CONFIG_SOC_SAMV71Q19B)
#include <samv71q19b.h>
#elif defined(CONFIG_SOC_SAMV71Q20B)
#include <samv71q20b.h>
#elif defined(CONFIG_SOC_SAMV71Q21B)
#include <samv71q21b.h>
#else
  #error Library does not support the specified device.
#endif

#include "../common/soc_pmc.h"
#include "../common/soc_gpio.h"
#include "../common/soc_supc.h"
#include "../common/atmel_sam_dt.h"

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM_HCLK_FREQ_HZ ATMEL_SAM_DT_CPU_CLK_FREQ_HZ

/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM_MCK_FREQ_HZ \
	(SOC_ATMEL_SAM_HCLK_FREQ_HZ / CONFIG_SOC_ATMEL_SAM_MDIV)

/** UTMI PLL clock (UPLLCK) Frequency */
#define SOC_ATMEL_SAM_UPLLCK_FREQ_HZ MHZ(480)

#endif /* _ASMLANGUAGE */

#endif /* _SOC_ATMEL_SAM_SAMX7X_SOC_H_ */
