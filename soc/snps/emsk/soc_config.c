/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/arch/cpu.h>
#include <zephyr/devicetree.h>
#include <zephyr/init.h>


#ifdef CONFIG_UART_NS16550

void soc_early_init_hook(void)
{

	/* On ARC EM Starter kit board,
	 * send the UART the command to clear the interrupt
	 */
#if DT_NODE_HAS_STATUS_OKAY(DT_INST(0, ns16550))
	sys_write32(0, DT_REG_ADDR(DT_INST(0, ns16550))+0x4);
	sys_write32(0, DT_REG_ADDR(DT_INST(0, ns16550))+0x10);
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_INST(0, ns16550)) */
#if DT_NODE_HAS_STATUS_OKAY(DT_INST(1, ns16550))
	sys_write32(0, DT_REG_ADDR(DT_INST(1, ns16550))+0x4);
	sys_write32(0, DT_REG_ADDR(DT_INST(1, ns16550))+0x10);
#endif /* DT_NODE_HAS_STATUS_OKAY(DT_INST(1, ns16550)) */
}

#endif /* CONFIG_UART_NS16550 */
