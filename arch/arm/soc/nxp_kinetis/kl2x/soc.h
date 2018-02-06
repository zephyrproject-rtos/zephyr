/*
 * Copyright (c) 2017, NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <misc/util.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UART0_CLK_SRC kCLOCK_CoreSysClk

/* IRQs */
#define IRQ_SPI0		10
#define IRQ_SPI1                11
#define IRQ_GPIO_PORTA          30
#define IRQ_GPIO_PORTD          31

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>
#include <random/rand32.h>

#endif /* !_ASMLANGUAGE */

#ifdef __cplusplus
}
#endif

#endif /* _SOC__H_ */
