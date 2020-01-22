/*
 * Copyright (c) 2020 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <device.h>
#include <soc.h>

#ifdef CONFIG_UART_PL011
extern void pl011_isr(void *arg);

void pl011_isr_wrapper(void *arg)
{
	pl011_isr(arg);
	EOSS3_ClearPendingIRQ(Uart_IRQn);
}
#endif

static int register_irq_wrappers(const struct device *arg)
{
#ifdef CONFIG_UART_PL011
#if DT_NUM_IRQS(DT_INST(0, arm_pl011)) == 1
	IRQ_REPLACE_ISR(DT_IRQN(DT_N_S_soc_S_uart_40010000),
			pl011_isr_wrapper);
#else
#error "EOS-S3 has only 1 UART peripheral and 1 common IRQ"
#endif
#endif
	return 0;
}

SYS_INIT(register_irq_wrappers, PRE_KERNEL_1, 0);
