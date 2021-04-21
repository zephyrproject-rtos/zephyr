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
#include <drivers/uart.h>

#define RCGC1 (*((volatile uint32_t *)0x400FE104))

#define RCGC1_UART0_EN 0x00000001
#define RCGC1_UART1_EN 0x00000002
#define RCGC1_UART2_EN 0x00000004

static int uart_stellaris_init(const struct device *dev)
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

#ifdef CONFIG_ETH_STELLARIS

#define RCGC2 (*((volatile uint32_t *)0x400FE108))

#define RCGC2_PHY_EN   0x40000000
#define RCGC2_EMAC_EN  0x10000000

static int eth_stellaris_init(const struct device *dev)
{
	RCGC2 |= (RCGC2_PHY_EN | RCGC2_EMAC_EN);
	return 0;
}

SYS_INIT(eth_stellaris_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);

#endif /* CONFIG_ETH_STELLARIS */
