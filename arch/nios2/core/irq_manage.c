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
#include <logging/kernel_event_logger.h>

void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	printk("Spurious interrupt detected! ipending: %x\n",
	       _nios2_creg_read(NIOS2_CR_IPENDING));
	_NanoFatalErrorHandler(_NANO_ERR_SPURIOUS_INT, &_default_esf);
}


void _arch_irq_enable(unsigned int irq)
{
	uint32_t ienable;
	int key;

	key = irq_lock();

	ienable = _nios2_creg_read(NIOS2_CR_IENABLE);
	ienable |= (1 << irq);
	_nios2_creg_write(NIOS2_CR_IENABLE, ienable);

	irq_unlock(key);
};



void _arch_irq_disable(unsigned int irq)
{
	uint32_t ienable;
	int key;

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
void _enter_irq(uint32_t ipending)
{
	int index;

	_kernel.nested++;

#ifdef CONFIG_IRQ_OFFLOAD
	_irq_do_offload();
#endif

	while (ipending) {
		struct _isr_table_entry *ite;

#ifdef CONFIG_KERNEL_EVENT_LOGGER_INTERRUPT
		_sys_k_event_logger_interrupt();
#endif

		index = find_lsb_set(ipending) - 1;
		ipending &= ~(1 << index);

		ite = &_sw_isr_table[index];
		ite->isr(ite->arg);
	}

	_kernel.nested--;
}

