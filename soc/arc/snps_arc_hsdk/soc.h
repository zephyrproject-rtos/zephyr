/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for HS Development Kit
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <misc/util.h>


/* ARC HS Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17
#define IRQ_ICI					19

#define BASE_ADDR_SYSCONFIG		0xF0000000

#define CREG_GPIO_MUX_BASE_ADDR		(BASE_ADDR_SYSCONFIG + 0x1484)

#ifndef _ASMLANGUAGE


#include <misc/util.h>
#include <random/rand32.h>

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define DT_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_1_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_2_IRQ_FLAGS	0 /* Default */

/* PINMUX IO Hardware Functions */
#define HSDK_PINMUX_FUNS		8

#define HSDK_PINMUX_FUN0		0
#define HSDK_PINMUX_FUN1		1
#define HSDK_PINMUX_FUN2		2
#define HSDK_PINMUX_FUN3		3
#define HSDK_PINMUX_FUN4		4
#define HSDK_PINMUX_FUN5		5
#define HSDK_PINMUX_FUN6		6
#define HSDK_PINMUX_FUN7		7

/* PINMUX MAX SELS */
#define HSDK_PINMUX_SELS		8

#define HSDK_PINMUX_SEL0		0
#define HSDK_PINMUX_SEL1		1
#define HSDK_PINMUX_SEL2		2
#define HSDK_PINMUX_SEL3		3
#define HSDK_PINMUX_SEL4		4
#define HSDK_PINMUX_SEL5		5
#define HSDK_PINMUX_SEL6		6
#define HSDK_PINMUX_SEL7		7


#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
