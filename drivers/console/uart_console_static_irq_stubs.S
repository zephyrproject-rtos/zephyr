/*
 * Copyright (c) 2015, Intel Corporation
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

/**
 * @file
 * @brief UART console interrupt stubs
 */

#ifdef CONFIG_X86

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>

#if defined(CONFIG_CONSOLE_HANDLER)
#if defined(CONFIG_IOAPIC) || defined(CONFIG_MVIC)
	ioapic_mkstub console uart_console_isr 0
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_UART_PIPE)
#if defined(CONFIG_IOAPIC) || defined(CONFIG_MVIC)
	ioapic_mkstub uart_pipe uart_pipe_isr 0
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_UART_PIPE */

#undef _ASMLANGUAGE

#endif /* CONFIG_X86 */
