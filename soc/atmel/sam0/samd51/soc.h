/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ATMEL_SAM0_SAMD51_SOC_H_
#define _SOC_ATMEL_SAM0_SAMD51_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAMD51G18A)
#include <samd51g18a.h>
#elif defined(CONFIG_SOC_SAMD51G19A)
#include <samd51g19a.h>
#elif defined(CONFIG_SOC_SAMD51J18A)
#include <samd51j18a.h>
#elif defined(CONFIG_SOC_SAMD51J19A)
#include <samd51j19a.h>
#elif defined(CONFIG_SOC_SAMD51J20A)
#include <samd51j20a.h>
#elif defined(CONFIG_SOC_SAMD51N19A)
#include <samd51n19a.h>
#elif defined(CONFIG_SOC_SAMD51N20A)
#include <samd51n20a.h>
#elif defined(CONFIG_SOC_SAMD51P19A)
#include <samd51p19a.h>
#elif defined(CONFIG_SOC_SAMD51P20A)
#include <samd51p20a.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include "sercom_fixup_samd5x.h"
#include "tc_fixup_samd5x.h"
#include "adc_fixup_sam0.h"
#include "../common/soc_port.h"
#include "../common/atmel_sam0_dt.h"

#define SOC_ATMEL_SAM0_OSC32K_FREQ_HZ 32768

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ SOC_ATMEL_SAM0_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ 48000000

#endif /* _SOC_ATMEL_SAM0_SAMD51_SOC_H_ */
