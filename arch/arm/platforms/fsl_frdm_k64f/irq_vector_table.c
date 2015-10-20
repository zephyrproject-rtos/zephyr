/* irq_vector_table.c - IRQ part of vector table */

/*
 * Copyright (c) 2014 Wind River Systems, Inc.
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

/*
 * DESCRIPTION
 * This file contains the IRQ part of the vector table. It is meant to be used
 * for one of two cases:
 *
 * a) When software-managed ISRs (SW_ISR_TABLE) is enabled, and in that case it
 *    binds _isr_wrapper() to all the IRQ entries in the vector table.
 *
 * b) When the platform is written so that device ISRs are installed directly in
 *    the vector table, they are enumerated here.
 */

#include <toolchain.h>
#include <sections.h>

#if defined(CONFIG_CONSOLE_HANDLER)
#include <board.h>
#include <console/uart_console.h>
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_BLUETOOTH_UART)
#include <board.h>
#include <bluetooth/uart.h>
#endif /* CONFIG_BLUETOOTH_UART */

extern void _isr_wrapper(void);
typedef void (*vth)(void); /* Vector Table Handler */

#if defined(CONFIG_SW_ISR_TABLE)

vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
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

#if defined(CONFIG_BLUETOOTH_UART)
static void _bt_uart_isr(void)
{
	bt_uart_isr(NULL);
	_IntExit();
}
#endif /* CONFIG_BLUETOOTH_UART */

/* placeholders: fill with real ISRs */
vth __irq_vector_table _irq_vector_table[CONFIG_NUM_IRQS] = {
	[0 ...(CONFIG_NUM_IRQS - 1)] = _irq_spurious,
#if defined(CONFIG_CONSOLE_HANDLER)
	[CONFIG_UART_CONSOLE_IRQ] = _uart_console_isr,
#endif
#if defined(CONFIG_BLUETOOTH_UART)
	[CONFIG_BLUETOOTH_UART_IRQ] = _bt_uart_isr,
#endif
};

#endif /* CONFIG_SW_ISR_TABLE */
