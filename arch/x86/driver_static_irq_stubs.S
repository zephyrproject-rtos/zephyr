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
by x86 BSPs.
*/

#define _ASMLANGUAGE

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/pic.h>
#include <drivers/system_timer.h>

#if defined (CONFIG_PIC)
	GTEXT(_pic_master__i8259_boi_master_stub)
	GTEXT(_pic_slave__i8259_boi_slave_stub)
#endif

#if defined (CONFIG_PIT)
	pic_master_mkstub i8253 _timer_int_handler
#endif

#if defined(CONFIG_LOAPIC_TIMER)
	loapic_mkstub loapic _timer_int_handler
#endif

#if defined(CONFIG_HPET_TIMER)
	ioapic_mkstub hpet _timer_int_handler
#endif

#if defined(CONFIG_BLUETOOTH_UART)
#if defined(CONFIG_PIC)
	pic_master_mkstub bluetooth bt_uart_isr
#elif defined(CONFIG_IOAPIC)
	ioapic_mkstub bluetooth bt_uart_isr
#endif /* CONFIG_PIC */
#endif /* CONFIG_BLUETOOTH_UART */

#if defined(CONFIG_CONSOLE_HANDLER)
#if defined(CONFIG_PIC)
	pic_master_mkstub console uart_console_isr
#elif defined(CONFIG_IOAPIC)
	ioapic_mkstub console uart_console_isr
#endif /* CONFIG_PIC */
#endif /* CONFIG_CONSOLE_HANDLER */

#if defined(CONFIG_UART_SIMPLE)
#if defined(CONFIG_PIC)
	pic_master_mkstub uart_simple uart_simple_isr
#elif defined(CONFIG_IOAPIC)
	ioapic_mkstub uart_simple uart_simple_isr
#endif /* CONFIG_PIC */
#endif /* CONFIG_UART_SIMPLE */

	/* externs (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExit)

#if defined(CONFIG_PIC)
SECTION_FUNC(TEXT, _pic_master__i8259_boi_master_stub)
	/*
	 * Handle possible spurious (stray) interrupts on IRQ 7. Since on this
	 * particular BSP, no device is hooked up to IRQ 7, a C level ISR is
	 * not called as the call to the BOI routine will not return.
	 */
	call	_IntEnt		   /* Inform kernel interrupt has begun */
	call	_i8259_boi_master  /* Call the BOI routine (won't return) */

	/*
	 * If an actual device was installed on IRQ 7, then the BOI may return,
	 * indicating a real interrupt was asserted on IRQ 7.
	 * The following code should be invoked in this case to invoke the ISR:
	 *
	 * pushl $param		/+ Push argument to ISR +/
	 * call	ISR		/+ Call 'C' level ISR +/
	 * addl	$4, %esp	/+ pop arg to ISR +/
	 * jmp	_IntExit	/+ Inform kernel interrupt is done +/
	 */

SECTION_FUNC(TEXT, _pic_slave__i8259_boi_slave_stub)
	/*
	 * Handle possible spurious (stray) interrupts on IRQ 15 (slave PIC
	 * IRQ 7). Since on this particular BSP, no device is hooked up to
	 * IRQ 15, a C level ISR is not called as the call the BOI routine
	 * will not return.
	 */
	call	_IntEnt		   /* Inform kernel interrupt has begun */
	call	_i8259_boi_slave   /* Call the BOI routine (won't return) */

	/*
	 * If an actual device was installed on IRQ 15, then the BOI may return,
	 * indicating a real interrupt was asserted on IRQ 15.
	 * The following code should be invoked in this case to invoke the ISR:
	 *
	 * pushl $param		/+ Push argument to ISR +/
	 * call	ISR		/+ Call 'C' level ISR +/
	 * addl	$4, %esp	/+ pop arg to ISR +/
	 * jmp	_IntExit	/+ Inform kernel interrupt is done +/
	 */
#endif /* CONFIG_PIC */
