/* board.h - board configuration macros for the 'Quark' BSP */

/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
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
This header file is used to specify and describe board-level aspects for
the 'Quark' BSP.
*/

#ifndef __INCboardh
#define __INCboardh

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <drivers/rand32.h>
#endif

#define N_PIC_IRQS 16     /* number of PIC IRQs */
#define INT_VEC_IRQ0 0x20 /* Vector number for IRQ0 */

/*
 * IO APIC (IOAPIC) device information (Intel ioapic)
 */
#define IOAPIC_NUM_RTES 24 /* Number of IRQs = 24 */

#define IOAPIC_BASE_ADRS_PHYS 0xFEC00000 /* base physical address */
#define IOAPIC_SIZE KB(4)

#define IOAPIC_BASE_ADRS IOAPIC_BASE_ADRS_PHYS

/*
 * Local APIC (LOAPIC) device information (Intel loapic)
 */

#define LOAPIC_BASE_ADRS_PHYS 0xFEE00000 /* base physical address */
#define LOAPIC_SIZE KB(4)

#define LOAPIC_BASE_ADRS LOAPIC_BASE_ADRS_PHYS
/*
 * HPET device information
 */

#define HPET_BASE_ADRS_PHYS 0xFED00000
#define HPET_SIZE KB(4)

#define HPET_BASE_ADRS HPET_BASE_ADRS_PHYS

#define HPET_TIMER0_IRQ (20)
#define HPET_TIMER0_VEC (HPET_TIMER0_IRQ + INT_VEC_IRQ0)
/* HPET uses falling edge triggered interrupt */
#define HPET_IOAPIC_FLAGS (IOAPIC_EDGE | IOAPIC_LOW)

/* serial port (aka COM port) information */
#define COM1_BAUD_RATE 115200
#define COM1_PCI_IDX 2

#define COM2_BAUD_RATE 115200
#define COM2_PCI_IDX 0
#define COM2_INT_LVL 0x11 /* COM2 connected to IRQ17 */

#define UART_REG_ADDR_INTERVAL 4 /* address diff of adjacent regs. */
#define UART_XTAL_FREQ (2764800 * 16)
/* UART uses level triggered interrupt, low level */
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)

/* uart configuration settings */

/* Generic definitions */
#define CONFIG_UART_NUM_SYSTEM_PORTS 2
#define CONFIG_UART_NUM_EXTRA_PORTS 0
#define CONFIG_UART_BAUDRATE COM1_BAUD_RATE
#define CONFIG_UART_NUM_PORTS \
	(CONFIG_UART_NUM_SYSTEM_PORTS + CONFIG_UART_NUM_EXTRA_PORTS)

/* Console definitions */
#define CONFIG_UART_CONSOLE_INDEX 0
#define CONFIG_UART_CONSOLE_PCI_IDX COM1_PCI_IDX

/*
 * The irq_connect() API connects to a (virtualized) IRQ and the
 * associated interrupt controller is programmed with the allocated vector.
 * The Quark board virtualizes IRQs as follows:
 *
 *   - The first IOAPIC_NUM_RTES IRQs are provided by the IOAPIC
 *   - The remaining IRQs are provided by the LOAPIC.
 *
 * Thus, for example, if the IOAPIC supports 24 IRQs:
 *
 *   - IRQ0 to IRQ23   map to IOAPIC IRQ0 to IRQ23
 *   - IRQ24 to IRQ29  map to LOAPIC LVT entries as follows:
 *
 *       IRQ24 -> LOAPIC_TIMER
 *       IRQ25 -> LOAPIC_THERMAL
 *       IRQ26 -> LOAPIC_PMC
 *       IRQ27 -> LOAPIC_LINT0
 *       IRQ28 -> LOAPIC_LINT1
 *       IRQ29 -> LOAPIC_ERROR
 */
#define LOAPIC_VEC_BASE(x) (x + INT_VEC_IRQ0 + IOAPIC_NUM_RTES)

#ifndef _ASMLANGUAGE
/*
 * The <pri> parameter is deliberately ignored. For this BSP, the macro just has
 * to make sure that unique vector numbers are generated.
 */
#define SYS_INT_REGISTER(s, irq, pri) \
	NANO_CPU_INT_REGISTER(s, INT_VEC_IRQ0 + (irq), 0)
#endif

/* PCI definitions */
#define PCI_CTRL_ADDR_REG 0xCF8
#define PCI_CTRL_DATA_REG 0xCFC

#define PCI_INTA 1
#define PCI_INTB 2
#define PCI_INTC 3
#define PCI_INTD 4

#ifndef _ASMLANGUAGE
/*
 * Device drivers utilize the macros PLB_BYTE_REG_WRITE() and
 * PLB_BYTE_REG_READ() to access byte-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_BYTE_REG_WRITE(data, address) \
	io_outByte(data, (unsigned int)address)
#define PLB_BYTE_REG_READ(address) io_inByte((unsigned int)address)

/*******************************************************************************
*
* io_outByte - output a byte to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a byte to the specified
* I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile void io_outByte(unsigned char data, unsigned int port)
{
	% mem data, port;
	!"ax", "dx" movl port, % edx movb data, % al outb % al, % dx
}
#elif defined(__GNUC__)
static inline void io_outByte(unsigned char data, unsigned int port)
{
	__asm__ volatile("outb	%%al, %%dx;\n\t" : : "a"(data), "d"(port));
}
#endif

/*******************************************************************************
*
* io_inByte - input a byte from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a byte from the specified
* I/O port.
*
* RETURNS: the byte read from the specified I/O port
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile unsigned char io_inByte(unsigned int port)
{
	% mem port;
	!"ax", "dx" movl port, % edx inb % dx, % al
}
#elif defined(__GNUC__)
static inline unsigned char io_inByte(unsigned int port)
{
	char retByte;

	__asm__ volatile("inb	%%dx, %%al;\n\t" : "=a"(retByte) : "d"(port));
	return retByte;
}
#endif

/*******************************************************************************
*
* outByte - output byte to memory location
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline void outByte(uint8_t data, uint32_t addr)
{
	*(volatile uint8_t *)addr = data;
}

/*******************************************************************************
*
* inByte - obtain byte value from memory location
*
* This function issues the 'move' instruction to read a byte from the specified
* memory address.
*
* RETURNS: the byte read from the specified memory address
*
* NOMANUAL
*/

static inline uint8_t inByte(uint32_t addr)
{
	return *((volatile uint8_t *)addr);
}

/*
 * Device drivers utilize the macros PLB_WORD_REG_WRITE() and
 * PLB_WORD_REG_READ() to access shortword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_WORD_REG_WRITE(data, address) \
	io_outWord(data, (unsigned int)address)
#define PLB_WORD_REG_READ(address) io_inWord((unsigned int)address)

/*******************************************************************************
*
* io_outWord - output a word to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a word to the
* specified I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile void io_outWord(unsigned short data, unsigned int port)
{
	% mem data, port;
	!"ax", "dx" movl port, % edx movw data, % ax outw % ax, % dx
}
#elif defined(__GNUC__)
static inline void io_outWord(unsigned short data, unsigned int port)
{
	__asm__ volatile("outw	%%ax, %%dx;\n\t" :  : "a"(data), "d"(port));
}
#endif

/*******************************************************************************
*
* io_inWord - input a word from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a word from the
* specified I/O port.
*
* RETURNS: the word read from the specified I/O port
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile unsigned short io_inWord(unsigned int port)
{
	% mem port;
	!"ax", "dx" movl port, % edx inw % dx, % ax
}
#elif defined(__GNUC__)
static inline unsigned short io_inWord(unsigned int port)
{
	unsigned short retWord;

	__asm__ volatile("inw	%%dx, %%ax;\n\t" : "=a"(retWord) : "d"(port));
	return retWord;
}
#endif

/*******************************************************************************
*
* outWord - output word to memory location
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline void outWord(uint16_t data, uint32_t addr)
{
	*(volatile uint16_t *)addr = data;
}

/*******************************************************************************
*
* inWord - obtain word value from memory location
*
* This function issues the 'move' instruction to read a word from the specified
* memory address.
*
* RETURNS: the word read from the specified memory address
*
* NOMANUAL
*/

static inline uint16_t inWord(uint32_t addr)
{
	return *((volatile uint16_t *)addr);
}

/*
 * Device drivers utilize the macros PLB_LONG_REG_WRITE() and
 * PLB_LONG_REG_READ() to access longword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_LONG_REG_WRITE(data, address) \
	io_outLong(data, (unsigned int)address)
#define PLB_LONG_REG_READ(address) io_inLong((unsigned int)address)

/*******************************************************************************
*
* io_outLong - output a long word to an IA-32 I/O port
*
* This function issues the 'out' instruction to write a long word to the
* specified I/O port.
*
* RETURNS: N/A
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile void io_outLong(unsigned int data, unsigned int port)
{
	% mem data, port;
	!"ax", "dx" movl port, % edx movl data, % eax outl % eax, % dx
}
#elif defined(__GNUC__)
static inline void io_outLong(unsigned int data, unsigned int port)
{
	__asm__ volatile("outl	%%eax, %%dx;\n\t" :  : "a"(data), "d"(port));
}
#endif

/*******************************************************************************
*
* io_inLong - input a long word from an IA-32 I/O port
*
* This function issues the 'in' instruction to read a long word from the
* specified I/O port.
*
* RETURNS: the long read from the specified I/O port
*
* NOMANUAL
*/

#if defined(__DCC__)
__asm volatile unsigned long io_inLong(unsigned int port)
{
	% mem port;
	!"ax", "dx" movl port, % edx inl % dx, % eax
}
#elif defined(__GNUC__)
static inline unsigned long io_inLong(unsigned int port)
{
	unsigned long retLong;

	__asm__ volatile("inl	%%dx, %%eax;\n\t" : "=a"(retLong) : "d"(port));
	return retLong;
}
#endif

/*******************************************************************************
*
* outLong - output long word to memory location
*
* RETURNS: N/A
*
* NOMANUAL
*/

static inline void outLong(uint32_t data, uint32_t addr)
{
	*(volatile uint32_t *)addr = data;
}

/*******************************************************************************
*
* inLong - obtain long word value from memory location
*
* This function issues the 'move' instruction to read a word from the specified
* memory address.
*
* RETURNS: the long word read from the specified memory address
*
* NOMANUAL
*/

static inline uint32_t inLong(uint32_t addr)
{
	return *((volatile uint32_t *)addr);
}
#endif /* !_ASMLANGUAGE */

/*******************************************************************************
*
* pci_pin2irq - convert PCI interrupt PIN to IRQ
*
* The routine uses "standard design consideration" and implies that
* INTA (pin 1) -> IRQ 16
* INTB (pin 2) -> IRQ 17
* INTC (pin 3) -> IRQ 18
* INTD (pin 4) -> IRQ 19
*
* RETURNS: IRQ number, -1 if the result is incorrect
*
*/

static inline int pci_pin2irq(int pin)
{
	if ((pin < PCI_INTA) || (pin > PCI_INTD))
		return -1;
	return N_PIC_IRQS + pin - 1;
}

/*******************************************************************************
*
* pci_irq2pin - convert IRQ to PCI interrupt pin
*
* RETURNS: pin number, -1 if the result is incorrect
*
*/

static inline int pci_irq2pin(int irq)
{
	if ((irq < N_PIC_IRQS) || (irq > N_PIC_IRQS + PCI_INTD - 1))
		return -1;
	return irq - N_PIC_IRQS + 1;
}

extern void _SysIntVecProgram(unsigned int vector, unsigned int);

#endif /* __INCboardh */
