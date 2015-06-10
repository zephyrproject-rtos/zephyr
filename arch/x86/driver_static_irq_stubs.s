/* driver_static_irq_stubs.s - interrupt stubs */

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

#ifndef CONFIG_DYNAMIC_INT_STUBS

#include <arch/x86/asm.h>
#include <drivers/ioapic.h>
#include <drivers/loapic.h>
#include <drivers/pic.h>
#include <drivers/system_timer.h>

	/* exports (internal APIs) */
#if defined(CONFIG_LOAPIC_TIMER)
	GTEXT(_loapic_timer_irq_stub)
#endif

#if defined(CONFIG_HPET_TIMER)
	GTEXT(_hpetIntStub)
#endif

#if defined (CONFIG_PIC)
	GTEXT(_masterStrayIntStub)
	GTEXT(_slaveStrayIntStub)
#endif

#if defined (CONFIG_PIT)
	GTEXT(_i8253_interrupt_stub)
#endif

#if defined(CONFIG_BLUETOOTH_UART)
	GTEXT(_bluetooth_uart_stub)
#endif /* CONFIG_BLUETOOTH */

#if defined(CONFIG_CONSOLE_HANDLER)
	GTEXT(_console_uart_stub)
#endif /* CONFIG_CONSOLE_HANDLER */

	/* externs (internal APIs) */

	GTEXT(_IntEnt)
	GTEXT(_IntExit)

#if defined(CONFIG_LOAPIC_TIMER)
SECTION_FUNC (TEXT, _loapic_timer_irq_stub)
	call    _IntEnt			/* Inform kernel interrupt has begun */
	pushl   $0			/* Push dummy parameter */
	call    _timer_int_handler	/* Call actual interrupt handler */
	call    _loapic_eoi		/* Inform loapic interrupt is done */
	addl    $4, %esp		/* Clean-up stack from push above */
	jmp     _IntExit		/* Inform kernel interrupt is done */
#endif /* CONFIG_LOAPIC_TIMER */

#if defined(CONFIG_HPET_TIMER)
SECTION_FUNC(TEXT, _hpetIntStub)
	call    _IntEnt			/* Inform kernel interrupt has begun */
	pushl   $0			/* Push dummy parameter */
	call    _timer_int_handler	/* Call actual interrupt handler */
	call    _ioapic_eoi		/* Inform ioapic interrupt is done */
	addl    $4, %esp		/* Clean-up stack from push above */
	jmp     _IntExit		/* Inform kernel interrupt is done */
#endif /* CONFIG_HPET_TIMER */

#if defined(CONFIG_PIC)
SECTION_FUNC(TEXT, _masterStrayIntStub)
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

SECTION_FUNC(TEXT, _slaveStrayIntStub)
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

#if defined(CONFIG_PIT)
SECTION_FUNC(TEXT, _i8253_interrupt_stub)
	call    _IntEnt			/* Inform kernel interrupt has begun */
	pushl   $0			/* Push dummy parameter */
	call    _timer_int_handler	/* Call actual interrupt handler */
	call    _i8259_eoi_master	/* Inform the PIC interrupt is done */
	addl    $4, %esp		/* Clean-up stack from push above */
	jmp     _IntExit		/* Inform kernel interrupt is done */
#endif /* CONFIG_PIT */

#if defined(CONFIG_BLUETOOTH_UART)
#if defined(CONFIG_PIC)
SECTION_FUNC(TEXT, _bluetooth_uart_stub)
	call	_IntEnt			/* Inform kernel interrupt has begun */
	pushl	$0			/* Push dummy parameter */
	call	bt_uart_isr		/* Call actual interrupt handler */
	call	_i8259_eoi_master	/* Inform the PIC interrupt is done */
	addl	$4, %esp		/* Clean-up stack from push above */
	jmp	_IntExit		/* Inform kernel interrupt is done */
#elif defined(CONFIG_IOAPIC)
SECTION_FUNC(TEXT, _bluetooth_uart_stub)
	call	_IntEnt			/* Inform kernel interrupt has begun */
	pushl	$0			/* Push dummy parameter */
	call	bt_uart_isr		/* Call actual interrupt handler */
	call	_ioapic_eoi		/* Inform the PIC interrupt is done */
	addl	$4, %esp		/* Clean-up stack from push above */
	jmp	_IntExit		/* Inform kernel interrupt is done */
#endif /* CONFIG_PIC */
#endif /* CONFIG_BLUETOOTH_UART */

#if defined(CONFIG_CONSOLE_HANDLER)

#if defined(CONFIG_PIC)
SECTION_FUNC(TEXT, _console_uart_stub)
	call	_IntEnt			/* Inform kernel interrupt has begun */
	pushl	$0			/* Push dummy parameter */
	call	uart_console_isr	/* Call actual interrupt handler */
	call	_i8259_eoi_master	/* Inform the PIC interrupt is done */
	addl	$4, %esp		/* Clean-up stack from push above */
	jmp	_IntExit		/* Inform kernel interrupt is done */
#elif defined(CONFIG_IOAPIC)
SECTION_FUNC(TEXT, _console_uart_stub)
	call	_IntEnt			/* Inform kernel interrupt has begun */
	pushl	$0			/* Push dummy parameter */
	call	uart_console_isr	/* Call actual interrupt handler */
	call	_ioapic_eoi		/* Inform the PIC interrupt is done */
	addl	$4, %esp		/* Clean-up stack from push above */
	jmp	_IntExit		/* Inform kernel interrupt is done */
#endif /* CONFIG_PIC */

#endif /* CONFIG_CONSOLE_HANDLER */

#endif /* !CONFIG_DYNAMIC_INT_STUBS */
