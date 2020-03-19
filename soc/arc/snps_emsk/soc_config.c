/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
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
#ifdef DT_INST_0_NS16550
	sys_write32(0, DT_INST_0_NS16550_BASE_ADDRESS+0x4);
	sys_write32(0, DT_INST_0_NS16550_BASE_ADDRESS+0x10);
#endif /* DT_INST_0_NS16550 */
#ifdef DT_INST_1_NS16550
	sys_write32(0, DT_INST_1_NS16550_BASE_ADDRESS+0x4);
	sys_write32(0, DT_INST_1_NS16550_BASE_ADDRESS+0x10);
#endif /* DT_INST_1_NS16550 */

	return 0;
}

SYS_INIT(uart_ns16550_init, PRE_KERNEL_1, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

#endif /* CONFIG_UART_NS16550 */
