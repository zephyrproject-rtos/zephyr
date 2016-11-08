/*
 * Copyright (c) 2015 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @file Board config file
 */

#include <device.h>
#include <init.h>

#include <nanokernel.h>

#include "soc.h"

#ifdef CONFIG_UART_STELLARIS
#include <uart.h>

#define RCGC1 (*((volatile uint32_t *)0x400FE104))

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
