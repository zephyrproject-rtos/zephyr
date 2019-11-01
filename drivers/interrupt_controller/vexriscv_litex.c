/*
 * Copyright (c) 2018 - 2019 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <kernel.h>
#include <arch/cpu.h>
#include <init.h>
#include <irq.h>
#include <device.h>
#include <zephyr.h>
#include <zephyr/types.h>

#define IRQ_MASK		DT_INST_0_VEXRISCV_INTC0_IRQ_MASK_BASE_ADDRESS
#define IRQ_PENDING		DT_INST_0_VEXRISCV_INTC0_IRQ_PENDING_BASE_ADDRESS

#define TIMER0_IRQ		DT_INST_0_LITEX_TIMER0_IRQ_0
#define UART0_IRQ		DT_INST_0_LITEX_UART0_IRQ_0

#define ETH0_IRQ		DT_INST_0_LITEX_ETH0_IRQ_0

static inline void vexriscv_litex_irq_setmask(u32_t mask)
{
	__asm__ volatile ("csrw %0, %1" :: "i"(IRQ_MASK), "r"(mask));
}

static inline u32_t vexriscv_litex_irq_getmask(void)
{
	u32_t mask;

	__asm__ volatile ("csrr %0, %1" : "=r"(mask) : "i"(IRQ_MASK));
	return mask;
}

static inline u32_t vexriscv_litex_irq_pending(void)
{
	u32_t pending;

	__asm__ volatile ("csrr %0, %1" : "=r"(pending) : "i"(IRQ_PENDING));
	return pending;
}

static inline void vexriscv_litex_irq_setie(u32_t ie)
{
	if (ie) {
		__asm__ volatile ("csrrs x0, mstatus, %0"
				:: "r"(SOC_MSTATUS_IEN));
	} else {
		__asm__ volatile ("csrrc x0, mstatus, %0"
				:: "r"(SOC_MSTATUS_IEN));
	}
}

static void vexriscv_litex_irq_handler(void *device)
{
	struct _isr_table_entry *ite;
	u32_t pending, mask, irqs;

	pending = vexriscv_litex_irq_pending();
	mask = vexriscv_litex_irq_getmask();
	irqs = pending & mask;

#ifdef CONFIG_LITEX_TIMER
	if (irqs & (1 << TIMER0_IRQ)) {
		ite = &_sw_isr_table[TIMER0_IRQ];
		ite->isr(ite->arg);
	}
#endif

#ifdef CONFIG_UART_INTERRUPT_DRIVEN
	if (irqs & (1 << UART0_IRQ)) {
		ite = &_sw_isr_table[UART0_IRQ];
		ite->isr(ite->arg);
	}
#endif

#ifdef CONFIG_ETH_LITEETH
	if (irqs & (1 << ETH0_IRQ)) {
		ite = (struct _isr_table_entry *)&_sw_isr_table[ETH0_IRQ];
		ite->isr(ite->arg);
	}
#endif
}

void z_arch_irq_enable(unsigned int irq)
{
	vexriscv_litex_irq_setmask(vexriscv_litex_irq_getmask() | (1 << irq));
}

void z_arch_irq_disable(unsigned int irq)
{
	vexriscv_litex_irq_setmask(vexriscv_litex_irq_getmask() & ~(1 << irq));
}

int z_arch_irq_is_enabled(unsigned int irq)
{
	return vexriscv_litex_irq_getmask() & (1 << irq);
}

static int vexriscv_litex_irq_init(struct device *dev)
{
	ARG_UNUSED(dev);
	__asm__ volatile ("csrrs x0, mie, %0"
			:: "r"((1 << RISCV_MACHINE_TIMER_IRQ)
				| (1 << RISCV_MACHINE_EXT_IRQ)));
	vexriscv_litex_irq_setie(1);
	IRQ_CONNECT(RISCV_MACHINE_EXT_IRQ, 0, vexriscv_litex_irq_handler,
			NULL, 0);

	return 0;
}

SYS_INIT(vexriscv_litex_irq_init, PRE_KERNEL_2,
		CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);
