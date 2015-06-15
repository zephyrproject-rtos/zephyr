/* i8259Boi.s - Intel 8259A PIC BOI Handler */

/*
 * Copyright (c) 2013-2015 Wind River Systems, Inc.
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

The PIC BOI handler determines if the IRQ in question is a spurious or real
interrupt. The IRQ inputs must remain high until after the falling edge of the
first INTA.  A spurious interrupt on IRQ 7 can occur if the IRQ input goes low
before this time when the CPU acknowledges the interrupt. In this case, the
interrupt handler should simply return without sending an EOI command.

The distinction between a spurious interrupt and a real one is detected by
looking at the in service register (ISR). The bit (bit 7) will be 1 indicating
a real IRQ has been inserted.

*/

/* includes */
#define _ASMLANGUAGE

#include <arch/cpu.h>
#include <arch/x86/asm.h>

#include <drivers/pic.h>
#include <board.h>

 	/* externs */

	GTEXT(_IntExit)
	GDATA(_i8259_spurious_interrupt_count)


/*******************************************************************************
*
* _i8259_boi_master - detect whether it is spurious interrupt or not
*
* This routine is called before the user's interrupt handler to detect the
* spurious interrupt on the master PIC.  If a spurious interrupt condition is
* detected, a global variable is incremented and the execution of the interrupt
* stub is "short circuited", i.e. a return to the interrupted context
* occurs.
*
* void _i8259_boi_master (void)
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _i8259_boi_master)
    /* disable interrupts */
    pushfl
    cli

    /* Master PIC, get contents of in serivce register */
    PLB_BYTE_REG_WRITE (0x0b, PIC_PORT1(PIC_MASTER_BASE_ADRS))
    PLB_BYTE_REG_READ (PIC_PORT1(PIC_MASTER_BASE_ADRS))

    /* enable interrupts */
    popfl

    /* Contents of ISR in %AL */
    andb    $0x80, %al
    je	    spur_isr

    ret


/*******************************************************************************
*
* _i8259_boi_slave - detect whether it is spurious interrupt or not
*
* This routine is called before the user's interrupt handler to detect the
* spurious interrupt on the slave PIC.  If a spurious interrupt condition is
* detected, a global variable is incremented and the execution of the interrupt
* stub is "short circuited", i.e. a return to the interrupted context
* occurs.
*
* void _i8259_boi_slave (void)
*
* RETURNS: N/A
*/

SECTION_FUNC(TEXT, _i8259_boi_slave)
    /* disable interrupts */
    pushfl
    cli

    /* Slave PIC, get contents of in serivce register */
    PLB_BYTE_REG_WRITE (0x0b, PIC_PORT1 (PIC_SLAVE_BASE_ADRS))
    PLB_BYTE_REG_READ (PIC_PORT1 (PIC_SLAVE_BASE_ADRS))

    /* Contents of ISR in EAX */
    testb   %al, %al
    jne	    check_isr

    /* Check the master PIC's in service register for slave PIC IRQ */
    PLB_BYTE_REG_WRITE (0x0b, PIC_PORT1(PIC_MASTER_BASE_ADRS))
    PLB_BYTE_REG_READ (PIC_PORT1(PIC_MASTER_BASE_ADRS))

    /* Slave connected to IRQ2 on master */
    testb   $0x4, %al
    je	    check_isr

    /* Send non-specific EOI to the master PIC IRQ2 */
    PLB_BYTE_REG_WRITE (I8259_EOI, PIC_IACK (PIC_MASTER_BASE_ADRS));

BRANCH_LABEL(check_isr)
    /* unlock interrupts */
    popfl

    /* Contents of ISR for either PIC in %AL */
    andb    $0x80, %al
    je	    spur_isr

    ret

BRANCH_LABEL(spur_isr)
    /* An actual spurious interrupt. Increment counter and short circuit */
    incl    _i8259_spurious_interrupt_count

    /* Pop the return address */
    addl    $4, %esp
    jmp	    _IntExit
