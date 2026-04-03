/*
 * Copyright (c) 2019 ML!PA Consulting GmbH
 * Copyright (c) 2024 Gerson Fernando Budke <nandojve@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_ATMEL_SAM0_SAME54_SOC_H_
#define _SOC_ATMEL_SAM0_SAME54_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_SAME54N19A)
#include <same54n19a.h>
#elif defined(CONFIG_SOC_SAME54N20A)
#include <same54n20a.h>
#elif defined(CONFIG_SOC_SAME54P19A)
#include <same54p19a.h>
#elif defined(CONFIG_SOC_SAME54P20A)
#include <same54p20a.h>
#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include "sercom_fixup_samd5x.h"
#include "tc_fixup_samd5x.h"
#include "gmac_fixup_samd5x.h"
#include "adc_fixup_sam0.h"
#include "../common/soc_port.h"
#include "../common/atmel_sam0_dt.h"

#define SOC_ATMEL_SAM0_OSC32K_FREQ_HZ 32768
#define SOC_ATMEL_SAM0_DFLL48_FREQ_HZ 48000000

/** Processor Clock (HCLK) Frequency */
#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ SOC_ATMEL_SAM0_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_ATMEL_SAM0_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ 48000000

#endif /* _SOC_ATMEL_SAM0_SAME54_SOC_H_ */
