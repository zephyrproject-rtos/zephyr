/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for EM Starter kit board
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <misc/util.h>

/* default system clock */
#define SYSCLK_DEFAULT_IOSC_HZ			MHZ(16)

/*
 * UART
 */
#define UART_NS16550_ACCESS_IOPORT


/* ARC EM Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17
#include "soc_irq.h"

#define BASE_ADDR_SYSCONFIG		0xF000A000

#ifndef _ASMLANGUAGE


#include <misc/util.h>
#include <random/rand32.h>

#define ARCV2_TIMER0_INT_LVL			IRQ_TIMER0
#define ARCV2_TIMER0_INT_PRI			0

#define ARCV2_TIMER1_INT_LVL			IRQ_TIMER1
#define ARCV2_TIMER1_INT_PRI			1

#define INT_ENABLE_ARC				~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS			(8)

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define CONFIG_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */
#define CONFIG_UART_NS16550_PORT_1_IRQ_FLAGS	0 /* Default */
#define CONFIG_UART_NS16550_PORT_2_IRQ_FLAGS	0 /* Default */


#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
