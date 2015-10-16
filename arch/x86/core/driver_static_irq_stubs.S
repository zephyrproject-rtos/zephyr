/* driver_static_irq_stubs.S - interrupt stubs */

/*
 * Copyright (c) 2012-2015, Wind River Systems, Inc.
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
DESCRIPTION
This module contains the static interrupt stubs for the various drivers employed
by x86 platforms.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/system_timer.h>

#if defined(CONFIG_LOAPIC_TIMER)
	loapic_mkstub loapic _timer_int_handler 0
#endif

#if defined(CONFIG_HPET_TIMER)
	ioapic_mkstub hpet _timer_int_handler 0
#endif

#if defined(CONFIG_BLUETOOTH_UART)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub bluetooth bt_uart_isr 0
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_BLUETOOTH_UART */

#if defined(CONFIG_CONSOLE_HANDLER)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub console uart_console_isr 0
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_UART_SIMPLE)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub uart_simple uart_simple_isr 0
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_UART_SIMPLE */

	/* externs (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExit)
