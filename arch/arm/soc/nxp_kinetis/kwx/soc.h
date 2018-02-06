/*
 * Copyright (c) 2017, NXP
 * Copyright (c) 2017, Phytec Messtechnik GmbH
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(CONFIG_SOC_MKW40Z4) || defined(CONFIG_SOC_MKW41Z4)

#define LPUART0_CLK_SRC kCLOCK_CoreSysClk

/* IRQs */

#define IRQ_SPI0 10
#define IRQ_SPI1 29
#define IRQ_GPIO_PORTA 30
#define IRQ_GPIO_PORTB 31
#define IRQ_GPIO_PORTC 31

#endif

#if defined(CONFIG_SOC_MKW22D5) || defined(CONFIG_SOC_MKW24D5)

#define PERIPH_ADDR_BASE_WDOG 0x40052000 /* Watchdog Timer module */

/* IRQs */
#define IRQ_SPI0 26
#define IRQ_SPI1 27
#define IRQ_GPIO_PORTA 59
#define IRQ_GPIO_PORTB 60
#define IRQ_GPIO_PORTC 61
#define IRQ_GPIO_PORTD 62
#define IRQ_GPIO_PORTE 63

#endif

#ifndef _ASMLANGUAGE

#include <fsl_common.h>
#include <device.h>
#include <misc/util.h>
#include <random/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
