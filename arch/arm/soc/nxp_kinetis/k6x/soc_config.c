/*
 * Copyright (c) 2015 Intel Corporation.
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
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
#include <fsl_common.h>

#ifdef CONFIG_UART_K20
#include <uart.h>
#include <console/uart_console.h>
#include <serial/uart_k20_priv.h>
#endif /* CONFIG_UART_K20 */

/*
 * UART configuration
 */

#ifdef CONFIG_UART_K20

static int uart_k20_init(struct device *dev)
{
	uint32_t scgc4;

	ARG_UNUSED(dev);

	/* Although it is possible to modify the bits through
	 * *sim directly, the following code saves about 20 bytes
	 * of ROM space, compared to direct modification.
	 */
	scgc4 = SIM->SCGC4;

#ifdef CONFIG_UART_K20_PORT_0
	scgc4 |= SIM_SCGC4_UART0(1);
#endif

#ifdef CONFIG_UART_K20_PORT_1
	scgc4 |= SIM_SCGC4_UART1(1);
#endif

#ifdef CONFIG_UART_K20_PORT_2
	scgc4 |= SIM_SCGC4_UART2(1);
#endif

#ifdef CONFIG_UART_K20_PORT_3
	scgc4 |= SIM_SCGC4_UART3(1);
#endif

	SIM->SCGC4 = scgc4;

#ifdef CONFIG_UART_K20_PORT_4
	SIM->SCGC1 |= SIM_SCGC1_UART4(1);
#endif

	return 0;
}

DEVICE_INIT(_uart_k20_init, "", uart_k20_init,
				NULL, NULL,
				PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_UART_K20 */
