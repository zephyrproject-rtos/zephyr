/*
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the ia32 platform
 *
 * This header file is used to specify and describe board-level aspects for
 * the 'ia32' platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

#define INT_VEC_IRQ0 0x20 /* vector number for IRQ0 */

/*
 * UART
 */
#define UART_NS16550_ACCESS_IOPORT

#define UART_NS16550_PORT_0_BASE_ADDR		0x03F8
#define UART_NS16550_PORT_0_IRQ			4
#define UART_NS16550_PORT_0_CLK_FREQ		1843200

#define UART_NS16550_PORT_1_BASE_ADDR		0x02F8
#define UART_NS16550_PORT_1_IRQ			3
#define UART_NS16550_PORT_1_CLK_FREQ		1843200

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#define UART_IRQ_FLAGS				(IOAPIC_EDGE | IOAPIC_HIGH)
#endif /* CONFIG_IOAPIC */

#endif /* __SOC_H_ */
