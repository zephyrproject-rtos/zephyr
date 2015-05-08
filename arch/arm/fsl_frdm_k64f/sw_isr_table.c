/* sw_isr_table.c - Software ISR table for fsl_frdm_k64f BSP */

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
This contains the ISR table meant to be used for ISRs that take a parameter.
It is also used when ISRs are to be connected at runtime, and in this case
provides a table that is filled with _irq_spurious bindings.
*/

#include <toolchain.h>
#include <sections.h>
#include <sw_isr_table.h>

extern void _irq_spurious(void *arg);

#if defined(CONFIG_CONSOLE_HANDLER)
#include <board.h>
#include <console/uart_console.h>
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_BLUETOOTH_UART)
#include <board.h>
#include <bluetooth/uart.h>
#endif /* CONFIG_BLUETOOTH_UART */

#if defined(CONFIG_SW_ISR_TABLE_DYNAMIC)

_IsrTableEntry_t __isr_table_section _sw_isr_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)].arg = (void *)0xABAD1DEA,
	[0 ...(CONFIG_NUM_IRQS - 1)].isr = _irq_spurious,
#if defined(CONFIG_CONSOLE_HANDLER)
	[CONFIG_UART_CONSOLE_IRQ].arg = NULL,
	[CONFIG_UART_CONSOLE_IRQ].isr = uart_console_isr,
#endif
};

#else
#if defined(CONFIG_SW_ISR_TABLE)
#if !defined(CONFIG_SW_ISR_TABLE_STATIC_CUSTOM)

/* placeholders: fill with real ISRs */

_IsrTableEntry_t __isr_table_section _sw_isr_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)].arg = (void *)0xABAD1DEA,
	[0 ...(CONFIG_NUM_IRQS - 1)].isr = _irq_spurious,
#if defined(CONFIG_CONSOLE_HANDLER)
	[CONFIG_UART_CONSOLE_IRQ].arg = NULL,
	[CONFIG_UART_CONSOLE_IRQ].isr = uart_console_isr,
#endif
#if defined(CONFIG_BLUETOOTH_UART)
	[CONFIG_BLUETOOTH_UART_IRQ].arg = NULL,
	[CONFIG_BLUETOOTH_UART_IRQ].isr = bt_uart_isr,
#endif
};

#endif /* !CONFIG_SW_ISR_TABLE_STATIC_CUSTOM */
#endif /* CONFIG_SW_ISR_TABLE */
#endif /* CONFIG_SW_ISR_TABLE_DYNAMIC */
