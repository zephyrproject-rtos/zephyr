/*
 * Copyright (c) 2024 Microchip
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC_MICROCHIP_PIC32CXSG61_SOC_H_
#define _SOC_MICROCHIP_PIC32CXSG61_SOC_H_

#ifndef _ASMLANGUAGE

#define DONT_USE_CMSIS_INIT

#include <zephyr/types.h>

#if defined(CONFIG_SOC_PIC32CX1025SG61100)
#include <pic32cx1025sg61100.h>
#elif defined(CONFIG_SOC_PIC32CX1025SG61128)
#include <pic32cx1025sg61128.h>

#else
#error Library does not support the specified device.
#endif

#endif /* _ASMLANGUAGE */

#include <pic32cxsg_fixups.h>
#include "../common/microchip_pic32cxsg_dt.h"

#define SOC_MICROCHIP_PIC32CXSG_OSC32K_FREQ_HZ 32768
#define SOC_MICROCHIP_PIC32CXSG_DFLL48_FREQ_HZ 48000000

/** Processor Clock (HCLK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
/** Master Clock (MCK) Frequency */
#define SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ   SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK0_FREQ_HZ SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_MICROCHIP_PIC32CXSG_GCLK2_FREQ_HZ 48000000

#define SOC_ATMEL_SAM0_HCLK_FREQ_HZ  CONFIG_SYS_CLOCK_HW_CYCLES_PER_SEC
#define SOC_ATMEL_SAM0_MCK_FREQ_HZ   SOC_MICROCHIP_PIC32CXSG_HCLK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK0_FREQ_HZ SOC_MICROCHIP_PIC32CXSG_MCK_FREQ_HZ
#define SOC_ATMEL_SAM0_GCLK2_FREQ_HZ 48000000

/* PIC32CXSG is using the original Atmel SAM0 UART driver.
 * The driver uses MCLK to select different GCLK register structures.
 * We define GCLK pointer using a HAL define fixup structure matching the MCLK
 * version of global clock registers.
 * Once PIC32CXSG has Microchip DFP compliant driver these will no longer be required.
 */
#define MCLK (DT_REG_ADDR(DT_NODELABEL(mclk)))
#define GCLK ((Gclk *)(DT_REG_ADDR(DT_NODELABEL(gclk))))

#endif /* _SOC_MICROCHIP_PIC32CXSG41_SOC_H_ */
