/* i8259.c - Intel 8259A PIC (Programmable Interrupt Controller) driver */

/*
 * Copyright (c) 2010-2015 Wind River Systems, Inc.
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
This module implements a VxMicro device driver for the Intel 8259A PIC
(Programmable Interrupt Controller).  In summary, the i8259A supports
up to 8 vectored priority interrupts.  Cascading up to 8 PICs allows support
of up to 64 vectored priority interrupts.  This device driver assumes two
cascaded 8259A (which is the standard configuration for desktop PC platforms).

This driver initializes the PICs to operate in fully nested mode, and
issues non-specified EOI mode (an exported routine that must be invoked
just prior to an ISR returning).

The following is an description of "Fully Nested Mode":
In this mode, interrupt requests are ordered in priority from 0 through 7
(0 is the highest priority).  When an interrupt is acknowledged, the highest
priority request is determined and its vector is placed on the bus.
Additionally, a bit of the Interrupt Service (IS) register is set.  This bit
remains set until the microprocessor issues an EOI command immediately before
returning from the interrupt service routine (ISR).  While the IS bit is set,
all further interrupts of the same or lower priority are inhibited, while
higher level interrupts are allowed.  The _i8259_eoi() routine is used by
the ISR to issue an EOI command.  The PICs in a PC typically operate
in this mode.  In this mode, while the slave PIC is being serviced by the
master PIC, the slave PIC blocks all higher priority interrupt requests.

Non-Specific EOI: When the 8259A is operated in the Fully Nested Mode, it
can determine which IS bit to reset on EOI.  When a non-specific EOI
command is issued, the 8259A will automatically reset the highest IS bit of
those that are set, since in the fully nested mode the highest IS level is
the last level acknowledged and serviced.

*/

/*
 * A board support package's board.h header must provide definitions for the
 * following constants:
 *
 *    PIC_REG_ADDR_INTERVAL
 *    PIC_MASTER_BASE_ADRS
 *    PIC_SLAVE_BASE_ADRS
 *    INT_VEC_IRQ0
 *
 * ...and the following register access macros:
 *
 *   PLB_BYTE_REG_WRITE
 *   PLB_BYTE_REG_READ
 */

/*
 * The _i8259_boi_master and _i8259_boi_slave functions are platform
 * specific and implemented in assembler
 */

/* includes */

#include <nanokernel.h>
#include <arch/cpu.h>
#include <toolchain.h>
#include <sections.h>

#include <drivers/pic.h>
#include <board.h>

/* defines */

#define OCW3_DEF 0x08 /* 3rd default control word */
#define OCW3_PCB 0x04 /* Polling Control Bit */
#define OCW3_ISR 0x03 /* Read in-service reg */
#define OCW3_IRR 0x02 /* Read inter request reg */

/* globals */

#ifndef CONFIG_SHUTOFF_PIC
unsigned int _i8259_spurious_interrupt_count =
	0; /* track # of spurious interrupts */

/*
 * The public interface for enabling/disabling a specific IRQ for the IA-32
 * architecture is defined as follows in arch/nanokernel/Intel/arch.h
 *
 *   extern void  irq_enable  (unsigned int irq);
 *   extern void  irq_disable (unsigned int irq);
 *
 * Provide the symbolic alias 'irq_enable' and 'irq_disable'
 * to the Intel 8259A device driver specific routines  _i8259_irq_enable()
 * and _i8259_irq_disable(), respectively.
 */

FUNC_ALIAS(_i8259_irq_enable, irq_enable, void);
FUNC_ALIAS(_i8259_irq_disable, irq_disable, void);
#endif /* CONFIG_SHUTOFF_PIC */

/*******************************************************************************
*
* _i8259_init - initialize the Intel 8259A PIC device driver
*
* This routine initializes the Intel 8259A PIC device driver and the device
* itself.
*
* RETURNS: N/A
*/

void _i8259_init(void)
{

	/*
	 * Initialize the Master PIC device.
	 *
	 * Whenever a command is issued with A0=0 and D4=1, this is interpreted
	 * as Initialization Command Word 1 (ICW1).
	 *   D0 = 1 (ICW4 required)
	 *   D1 = 0 (Cascaded PIC configuration)
	 *   D2 = X (ADI: only used in MCS-80/85 mode)
	 *   D3 = 0 (level triggered interrupt mode: edge triggered)
	 *   D4 = 1 (initiates initialization sequence: see above)
	 *   D5 -> D7 = X (only used in MCS-80/85 mode)
	 */

	PLB_BYTE_REG_WRITE(0x11, PIC_PORT1(PIC_MASTER_BASE_ADRS));

	/*
	 * ICW2 = upper 5 bits of vector presented by 8259 during /INTA cycle
	 */

	PLB_BYTE_REG_WRITE(INT_VEC_IRQ0, PIC_PORT2(PIC_MASTER_BASE_ADRS));

	/*
	 * ICW3 (Master): indicate which IRQ has slave connection.  On PC
	 * systems
	 * the slave PIC is connect to IRQ2.
	 */

	PLB_BYTE_REG_WRITE(0x04, PIC_PORT2(PIC_MASTER_BASE_ADRS));

	/*
	 * ICW4
	 * D0 = 1 (Mode: 0=8085, 1=8086)
	 * D1 = 0 (AEOI: 1=Auto End of Interrupt, 0=Normal)
	 * D2 = X (Master/Slave in buffered mode: 1=Master, 0=Slave)
	 * D3 = 0 (Buffer mode: 0 = non-buffered mode)
	 * D4 = 0 (SFNM: 1= Special fully nested mode)
	 */

	PLB_BYTE_REG_WRITE(0x01, PIC_PORT2(PIC_MASTER_BASE_ADRS));

	/*
	 * Initialize the Slave PIC device.
	 */

	PLB_BYTE_REG_WRITE(0x11, PIC_PORT1(PIC_SLAVE_BASE_ADRS)); /* ICW1 */
	PLB_BYTE_REG_WRITE(INT_VEC_IRQ0 + 8, PIC_PORT2(PIC_SLAVE_BASE_ADRS));
	PLB_BYTE_REG_WRITE(0x02, PIC_PORT2(PIC_SLAVE_BASE_ADRS)); /* ICW3 */
	PLB_BYTE_REG_WRITE(0x01, PIC_PORT2(PIC_SLAVE_BASE_ADRS)); /* ICW4 */

	/* disable interrupts */

	PLB_BYTE_REG_WRITE(0xfb, PIC_IMASK(PIC_MASTER_BASE_ADRS));
	PLB_BYTE_REG_WRITE(0xff, PIC_IMASK(PIC_SLAVE_BASE_ADRS));
}

#ifndef CONFIG_SHUTOFF_PIC
/*******************************************************************************
*
* _i8259_eoi_master - send EOI(end of interrupt) signal to the master PIC.
*
* This routine is called at the end of the interrupt handler.
*
* RETURNS: N/A
*
* ERRNO
*/

void _i8259_eoi_master(unsigned int irq /* IRQ number to
						   send EOI: unused */
				)
{
	ARG_UNUSED(irq);

	/* no need to disable interrupts since only writing to a single port */

	PLB_BYTE_REG_WRITE(I8259_EOI, PIC_IACK(PIC_MASTER_BASE_ADRS));
}

/*******************************************************************************
*
* _i8259_eoi_slave - send EOI(end of interrupt) signal to the slave PIC.
*
* This routine is called at the end of the interrupt handler in the Normal
* Fully Nested Mode.
*
* RETURNS: N/A
*
* ERRNO
*/

void _i8259_eoi_slave(unsigned int irq /* IRQ number to
						  send EOI: unused */
			       )
{
	ARG_UNUSED(irq);

	/* lock interrupts */

	__asm__ volatile(
		"pushfl;\n\t"
		"cli;\n\t");

	PLB_BYTE_REG_WRITE(I8259_EOI, PIC_IACK(PIC_SLAVE_BASE_ADRS));
	PLB_BYTE_REG_WRITE(I8259_EOI, PIC_IACK(PIC_MASTER_BASE_ADRS));

	/* unlock interrupts */

	__asm__ volatile("popfl;\n\t");
}

/*******************************************************************************
*
* __I8259IntEnable - enable/disable a specified PIC interrupt input line
*
* This routine enables or disables a specified PIC interrupt input line. To
* enable an interrupt input line, the parameter <enable> must be non-zero.
*
* The VxMicro nanokernel exports the irq_enable() and irq_disable()
* APIs (mapped to _i8259_irq_enable() and _i8259_irq_disable(), respectively).
* This function is called by _i8259_irq_enable() and _i8259_irq_disable() to
* perform the actual enabling/disabling of an IRQ to minimize footprint.
*
* RETURNS: N/A
*
* see also: _i8259_irq_disable()/_i8259_irq_enable
*/

static void __I8259IntEnable(
	unsigned int irq,    /* IRQ number to enable */
	unsigned char enable /* 0 = disable, otherwise enable */
	)
{
	unsigned char imask;
	unsigned char *picBaseAdrs;

	if (irq < 8)
		picBaseAdrs = (unsigned char *)PIC_MASTER_BASE_ADRS;
	else
		picBaseAdrs = (unsigned char *)PIC_SLAVE_BASE_ADRS;

	/*
	 * BSPs that utilize this interrupt controller driver virtualize IRQs
	 * as follows:
	 *
	 *   - IRQ0 to IRQ7  are provided by the master i8259 PIC
	 *   - IRQ8 to IRQ15 are provided by the slave i8259 PIC
	 *
	 * Thus translate the virtualized IRQ parameter to a physical IRQ
	 */

	irq %= 8;

	imask = PLB_BYTE_REG_READ(PIC_IMASK(picBaseAdrs));

	if (enable == 0)
		PLB_BYTE_REG_WRITE(imask | (1 << irq), PIC_IMASK(picBaseAdrs));
	else
		PLB_BYTE_REG_WRITE(imask & ~(1 << irq), PIC_IMASK(picBaseAdrs));

}

/*******************************************************************************
*
* _i8259_irq_disable - disable a specified PIC interrupt input line
*
* This routine disables a specified PIC interrupt input line.
*
* RETURNS: N/A
*
* SEE ALSO: _i8259_irq_enable()
*/

void _i8259_irq_disable(unsigned int irq /* IRQ number to disable */
				 )
{
	return __I8259IntEnable(irq, 0);
}

/*******************************************************************************
*
* _i8259_irq_enable - enable a specified PIC interrupt input line
*
* This routine enables a specified PIC interrupt input line.
*
* RETURNS: N/A
*
* SEE ALSO: _i8259_irq_disable()
*/

void _i8259_irq_enable(unsigned int irq /* IRQ number to enable */
				)
{
	return __I8259IntEnable(irq, 1);
}

#endif /* CONFIG_SHUTOFF_PIC */
