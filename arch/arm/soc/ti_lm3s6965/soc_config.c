/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file Board config file
 */

#include <device.h>
#include <init.h>

#include <kernel.h>

#include "soc.h"

#ifdef CONFIG_UART_STELLARIS
#include <uart.h>

#define RCGC1 (*((volatile u32_t *)0x400FE104))

#define RCGC1_UART0_EN 0x00000001
#define RCGC1_UART1_EN 0x00000002
#define RCGC1_UART2_EN 0x00000004

static int uart_stellaris_init(struct device *dev)
{
#ifdef CONFIG_UART_STELLARIS_PORT_0
	RCGC1 |= RCGC1_UART0_EN;
#endif

#ifdef CONFIG_UART_STELLARIS_PORT_1
	RCGC1 |= RCGC1_UART1_EN;
#endif

#ifdef CONFIG_UART_STELLARIS_PORT_2
	RCGC1 |= RCGC1_UART2_EN;
#endif

	return 0;
}

SYS_INIT(uart_stellaris_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_UART_STELLARIS */
