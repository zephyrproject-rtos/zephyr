/* driver_static_irq_stubs.S - interrupt stubs */

/*
 * Copyright (c) 2012-2015, Wind River Systems, Inc.
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
This module contains the static interrupt stubs for the various drivers employed
by x86 platforms.
 */

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/system_timer.h>

#if defined(CONFIG_LOAPIC_TIMER)
	loapic_mkstub loapic _timer_int_handler
#endif

#if defined(CONFIG_HPET_TIMER)
	ioapic_mkstub hpet _timer_int_handler
#endif

#if defined(CONFIG_BLUETOOTH_UART)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub bluetooth bt_uart_isr
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_BLUETOOTH_UART */

#if defined(CONFIG_CONSOLE_HANDLER)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub console uart_console_isr
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_UART_SIMPLE)
#if defined(CONFIG_IOAPIC)
	ioapic_mkstub uart_simple uart_simple_isr
#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_UART_SIMPLE */

#if defined(CONFIG_GPIO_DW)

#if CONFIG_GPIO_DW_0
#if defined(CONFIG_PIC)
	pic_master_mkstub gpio_dw_0 gpio_dw_isr_0
#elif defined(CONFIG_IOAPIC)
	ioapic_mkstub gpio_dw_0 gpio_dw_isr_0
#endif /* CONFIG_PIC */
#endif /* CONFIG_GPIO_DW_0 */

#if CONFIG_GPIO_DW_1
#if defined(CONFIG_PIC)
	pic_master_mkstub gpio_dw_1 gpio_dw_isr_1
#elif defined(CONFIG_IOAPIC)
	ioapic_mkstub gpio_dw_1 gpio_dw_isr_1
#endif /* CONFIG_PIC */
#endif /* CONFIG_GPIO_DW_1 */

#endif /* CONFIG_GPIO_DW */

#if defined(CONFIG_SPI_INTEL)
#if defined(CONFIG_PIC)
#if defined(CONFIG_SPI_INTEL_PORT_0)
	pic_master_mkstub spi_intel_irq_port_0 spi_intel_isr_0
#endif /* CONFIG_SPI_INTEL_PORT_0 */
#if defined(CONFIG_SPI_INTEL_PORT_1)
	pic_master_mkstub spi_intel_irq_port_1 spi_intel_isr_1
#endif /* CONFIG_SPI_INTEL_PORT_1 */
#elif defined(CONFIG_IOAPIC)
#if defined(CONFIG_SPI_INTEL_PORT_0)
	ioapic_mkstub spi_intel_irq_port_0 spi_intel_isr_0
#endif /* CONFIG_SPI_INTEL_PORT_0 */
#if defined(CONFIG_SPI_INTEL_PORT_1)
	ioapic_mkstub spi_intel_irq_port_1 spi_intel_isr_1
#endif /* CONFIG_SPI_INTEL_PORT_1 */
#endif
#endif /* CONFIG_SPI_INTEL */

	/* externs (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExit)
