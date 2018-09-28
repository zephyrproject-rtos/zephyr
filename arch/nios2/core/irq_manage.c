/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Nios II C-domain interrupt management code for use with Internal
 * Interrupt Controller (IIC)
 */


#include <kernel.h>
#include <kernel_structs.h>
#include <arch/cpu.h>
#include <irq.h>
#include <misc/printk.h>
#include <sw_isr_table.h>
#include <ksched.h>
#include <kswap.h>
#include <tracing.h>

void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("Spurious interrupt detected! ipending: %x\n",
	       _nios2_creg_read(NIOS2_CR_IPENDING));
	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}


void _arch_irq_enable(unsigned int irq)
{
	u32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = _nios2_creg_read(NIOS2_CR_IENABLE);
	ienable |= (1 << irq);
	_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};



void _arch_irq_disable(unsigned int irq)
{
	u32_t ienable;
	unsigned int key;

	key = irq_lock();

	ienable = _nios2_creg_read(NIOS2_CR_IENABLE);
	ienable &= ~(1 << irq);
	_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};


/**
 * @brief Interrupt demux function
 *
 * Given a bitfield of pending interrupts, execute the appropriate handler
 *
 * @param ipending Bitfield of interrupts
 */
void _enter_irq(u32_t ipending)
{
	int index;

#ifdef CONFIG_EXECUTION_BENCHMARKING
	extern void read_timer_start_of_isr(void);
	read_timer_start_of_isr();
#endif

	_kernel.nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

		z_sys_trace_isr_enter();

		index = find_lsb_set(ipending) - 1;
		ipending &= ~(1 << index);

		ite = &_sw_isr_table[index];

#ifdef CONFIG_EXECUTION_BENCHMARKING
		extern void read_timer_end_of_isr(void);
		read_timer_end_of_isr();
#endif
		ite->isr(ite->arg);
		sys_trace_isr_exit();
	}

	_kernel.nested--;
#ifdef CONFIG_STACK_SENTINEL
	_check_stack_sentinel();
#endif
}

