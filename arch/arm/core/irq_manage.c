/* irq_manage.c - ARM CORTEX-M3 interrupt management */

/*
 * Copyright (c) 2013-2014 Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION

Interrupt management: enabling/disabling and dynamic ISR connecting/replacing.
SW_ISR_TABLE_DYNAMIC has to be enabled for connecting ISRs at runtime.
 */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <misc/__assert.h>
#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>

extern void __reserved(void);

/**
 *
 * @brief Replace an interrupt handler by another
 *
 * An interrupt's ISR can be replaced at runtime. Care must be taken that the
 * interrupt is disabled before doing this.
 *
 * This routine will hang if <old> is not found in the table and ASSERT_ON is
 * enabled.
 *
 * @return N/A
 */

void irq_handler_set(unsigned int irq,
						void (*old)(void *arg),
						void (*new)(void *arg),
						void *arg)
{
	int key = irq_lock_inline();

	__ASSERT(old == _sw_isr_table[irq].isr, "expected ISR not found in table");

	if (old == _sw_isr_table[irq].isr) {
		_sw_isr_table[irq].isr = new;
		_sw_isr_table[irq].arg = arg;
	}

	irq_unlock_inline(key);
}

/**
 *
 * @brief Enable an interrupt line
 *
 * Clear possible pending interrupts on the line, and enable the interrupt
 * line. After this call, the CPU will receive interrupts for the specified
 * <irq>.
 *
 * @return N/A
 */

void irq_enable(unsigned int irq)
{
	/* before enabling interrupts, ensure that interrupt is cleared */
	_NvicIrqUnpend(irq);
	_NvicIrqEnable(irq);
}

/**
 *
 * @brief Disable an interrupt line
 *
 * Disable an interrupt line. After this call, the CPU will stop receiving
 * interrupts for the specified <irq>.
 *
 * @return N/A
 */

void irq_disable(unsigned int irq)
{
	_NvicIrqDisable(irq);
}

/**
 *
 * @brief Set an interrupt's priority
 *
 * Valid values are from 1 to 255. Interrupts of priority 1 are not masked when
 * interrupts are locked system-wide, so care must be taken when using them. ISR
 * installed with priority 1 interrupts cannot make kernel calls.
 *
 * Priority 0 is reserved for kernel usage and cannot be used.
 *
 * The priority is verified if ASSERT_ON is enabled.
 *
 * @return N/A
 */

void irq_priority_set(unsigned int irq,
					     unsigned int prio)
{
	__ASSERT(prio > 0 && prio < 256, "invalid priority!");
	_NvicIrqPrioSet(irq, _EXC_PRIO(prio));
}

/**
 *
 * @brief Spurious interrupt handler
 *
 * Installed in all dynamic interrupt slots at boot time. Throws an error if
 * called.
 *
 * See __reserved().
 *
 * @return N/A
 */

void _irq_spurious(void *unused)
{
	ARG_UNUSED(unused);
	__reserved();
}

/**
 *
 * @brief Connect an ISR to an interrupt line
 *
 * <isr> is connected to interrupt line <irq> (exception #<irq>+16). No prior
 * ISR can have been connected on <irq> interrupt line since the system booted.
 *
 * This routine will hang if another ISR was connected for interrupt line <irq>
 * and ASSERT_ON is enabled; if ASSERT_ON is disabled, it will fail silently.
 *
 * @return the interrupt line number
 */

int irq_connect(unsigned int irq,
					    unsigned int prio,
					    void (*isr)(void *arg),
					    void *arg)
{
	irq_handler_set(irq, _irq_spurious, isr, arg);
	irq_priority_set(irq, prio);
	return irq;
}

/**
 *
 * @brief Disconnect an ISR from an interrupt line
 *
 * Interrupt line <irq> (exception #<irq>+16) is disconnected from its ISR and
 * the latter is replaced by _irq_spurious(). irq_disable() should have
 * been called before invoking this routine.
 *
 * @return N/A
 */

void irq_disconnect(unsigned int irq)
{
	irq_handler_set(irq, _sw_isr_table[irq].isr, _irq_spurious, NULL);
}
