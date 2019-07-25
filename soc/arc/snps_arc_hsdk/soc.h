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

#define BASE_ADDR_SYSCONFIG		0xF000A000

#ifndef _ASMLANGUAGE


#include <misc/util.h>
#include <random/rand32.h>

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define DT_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_1_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_2_IRQ_FLAGS	0 /* Default */


#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
