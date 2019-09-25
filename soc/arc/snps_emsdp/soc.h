/*
 * Copyright (c) 2019 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for EM Software Development Platform board
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC_H_
#define _SOC_H_

#include <misc/util.h>

/* default system clock */
#define SYSCLK_DEFAULT_IOSC_HZ			MHZ(100)

/* ARC EM Core IRQs */
#define IRQ_TIMER0				16

#ifndef _ASMLANGUAGE


#include <misc/util.h>
#include <random/rand32.h>

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define DT_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */



#endif /* !_ASMLANGUAGE */

#endif /* _SOC_H_ */
