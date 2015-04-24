/* irq_vector_table.c - IRQ part of vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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
This file contains the IRQ part of the vector table. It is meant to be used
for one of two cases:

a) When software-managed ISRs (SW_ISR_TABLE) is enabled, and in that case it
   binds _isr_wrapper() to all the IRQ entries in the vector table.

b) When the BSP is written so that device ISRs are installed directly in the
   vector table, they are enumerated here.
*/

#include <toolchain.h>
#include <sections.h>

#if defined(CONFIG_CONSOLE_HANDLER)
#include <board.h>
#include <console/uart_console.h>
#endif /* CONFIG_CONSOLE_HANDLER */

extern void _isr_wrapper(void);
typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_SW_ISR_TABLE)

vth __irq_vector_table _IrqVectorTable[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _isr_wrapper,
};

#elif !defined(CONFIG_IRQ_VECTOR_TABLE_CUSTOM)

extern void _irq_spurious(void);

#if defined(CONFIG_CONSOLE_HANDLER)
static void _uart_console_isr(void)
{
	uart_console_isr(NULL);
	_IntExit();
}
#endif /* CONFIG_CONSOLE_HANDLER */

/* placeholders: fill with real ISRs */
vth __irq_vector_table _IrqVectorTable[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _irq_spurious,
#if defined(CONFIG_CONSOLE_HANDLER)
	[CONFIG_UART_CONSOLE_IRQ] = _uart_console_isr,
#endif
};

#endif /* CONFIG_SW_ISR_TABLE */
