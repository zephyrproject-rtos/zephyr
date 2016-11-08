/*
 * Copyright (c) 2016 Synopsys, Inc. All rights reserved.
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

#include <device.h>
#include <init.h>
#include "soc.h"


#ifdef CONFIG_UART_NS16550

static int uart_ns16550_init(struct device *dev)
{
	ARG_UNUSED(dev);

	/* On ARC EM Starter kit board,
	 * send the UART the command to clear the interrupt
	 */
#ifdef CONFIG_UART_NS16550_PORT_0
	sys_write32(0, UART_NS16550_PORT_0_BASE_ADDR+0x4);
	sys_write32(0, UART_NS16550_PORT_0_BASE_ADDR+0x10);
#endif /* CONFIG_UART_NS16550_PORT_0 */
#ifdef CONFIG_UART_NS16550_PORT_1
	sys_write32(0, UART_NS16550_PORT_1_BASE_ADDR+0x4);
	sys_write32(0, UART_NS16550_PORT_1_BASE_ADDR+0x10);
#endif /* CONFIG_UART_NS16550_PORT_1 */

	return 0;
}

SYS_INIT(uart_ns16550_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_UART_NS16550 */
