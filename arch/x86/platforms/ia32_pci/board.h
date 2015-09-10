/* board.h - board configuration macros for the ia32_pci platform */

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
the 'ia32_pci' platform.
 */

#ifndef __INCboardh
#define __INCboardh

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <drivers/rand32.h>
#endif

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#ifdef CONFIG_SERIAL_INTERRUPT_LEVEL
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#else
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL)
#endif
#else /* edge triggered interrupt */
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
/* generate interrupt on falling edge */
#define UART_IOAPIC_FLAGS (IOAPIC_LOW)
#else
/* generate interrupt on raising edge */
#define UART_IOAPIC_FLAGS (0)
#endif
#endif
#endif

#define NUM_STD_IRQS 16   /* number of "standard" IRQs on an x86 platform */
#define INT_VEC_IRQ0 0x20 /* Vector number for IRQ0 */

/* serial port (aka COM port) information */
#define COM1_BAUD_RATE 115200

#define COM2_BAUD_RATE 115200
#define COM2_INT_LVL 0x11 /* COM2 connected to IRQ17 */

#define UART_REG_ADDR_INTERVAL 4 /* address diff of adjacent regs. */
#define UART_XTAL_FREQ (2764800 * 16)

/* uart configuration settings */

/* Generic definitions */
#define CONFIG_UART_PCI_VENDOR_ID 0x8086
#define CONFIG_UART_PCI_DEVICE_ID 0x0936
#define CONFIG_UART_PCI_BUS 0
#define CONFIG_UART_PCI_DEV 20
#define CONFIG_UART_PORT_0_FUNCTION 1
#define CONFIG_UART_PORT_1_FUNCTION 5
#define CONFIG_UART_PCI_BAR       0
#define CONFIG_UART_BAUDRATE COM1_BAUD_RATE

#ifndef _ASMLANGUAGE
extern struct device * const uart_devs[];
#endif

#if defined(CONFIG_UART_CONSOLE)

#define CONFIG_UART_CONSOLE_IRQ		COM2_INT_LVL
#define CONFIG_UART_CONSOLE_INT_PRI	3

#define UART_CONSOLE_DEV (uart_devs[CONFIG_UART_CONSOLE_INDEX])

#endif /* CONFIG_UART_CONSOLE */

/* Bluetooth UART definitions */
#if defined(CONFIG_BLUETOOTH_UART)

#define CONFIG_BLUETOOTH_UART_INDEX	1
#define CONFIG_BLUETOOTH_UART_IRQ	COM2_INT_LVL
#define CONFIG_BLUETOOTH_UART_INT_PRI	3
#define CONFIG_BLUETOOTH_UART_FREQ	UART_XTAL_FREQ
#define CONFIG_BLUETOOTH_UART_BAUDRATE	CONFIG_UART_BAUDRATE
#define BT_UART_DEV		(uart_devs[CONFIG_BLUETOOTH_UART_INDEX])

#endif /* CONFIG_BLUETOOTH_UART */

#ifdef CONFIG_DW_I2C0

#include <drivers/ioapic.h>

#if defined(CONFIG_DW_I2C0_IRQ_FALLING_EDGE)
#define DW_I2C0_IRQ_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_DW_I2C0_IRQ_RISING_EDGE)
#define DW_I2C0_IRQ_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_DW_I2C0_IRQ_LEVEL_HIGH)
#define DW_I2C0_IRQ_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_DW_I2C0_IRQ_LEVEL_LOW)
#define DW_I2C0_IRQ_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#endif /* CONFIG_DW_I2C0 */

/*
 * The irq_connect() API connects to a (virtualized) IRQ and the
 * associated interrupt controller is programmed with the allocated vector.
 * The Quark board virtualizes IRQs as follows:
 *
 *   - The first CONFIG_IOAPIC_NUM_RTES IRQs are provided by the IOAPIC
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

/* PCI definitions */
#define PCI_BUS_NUMBERS 2

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
	sys_out8(data, (unsigned int)address)
#define PLB_BYTE_REG_READ(address) sys_in8((unsigned int)address)

/*
 * Device drivers utilize the macros PLB_WORD_REG_WRITE() and
 * PLB_WORD_REG_READ() to access shortword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_WORD_REG_WRITE(data, address) \
	sys_out16(data, (unsigned int)address)
#define PLB_WORD_REG_READ(address) sys_in16((unsigned int)address)

/*
 * Device drivers utilize the macros PLB_LONG_REG_WRITE() and
 * PLB_LONG_REG_READ() to access longword-wide registers on the processor
 * local bus (PLB), as opposed to a PCI bus, for example.  Boards are
 * expected to provide implementations of these macros.
 */

#define PLB_LONG_REG_WRITE(data, address) \
	sys_out32(data, (unsigned int)address)
#define PLB_LONG_REG_READ(address) sys_in32((unsigned int)address)

#endif /* !_ASMLANGUAGE */

/**
 *
 * @brief Convert PCI interrupt PIN to IRQ
 *
 * The routine uses "standard design consideration" and implies that
 * INTA (pin 1) -> IRQ 16
 * INTB (pin 2) -> IRQ 17
 * INTC (pin 3) -> IRQ 18
 * INTD (pin 4) -> IRQ 19
 *
 * @return IRQ number, -1 if the result is incorrect
 *
 */

static inline int pci_pin2irq(int pin)
{
	if ((pin < PCI_INTA) || (pin > PCI_INTD))
		return -1;
	return NUM_STD_IRQS + pin - 1;
}

/**
 *
 * @brief Convert IRQ to PCI interrupt pin
 *
 * @return pin number, -1 if the result is incorrect
 *
 */

static inline int pci_irq2pin(int irq)
{
	if ((irq < NUM_STD_IRQS) || (irq > NUM_STD_IRQS + PCI_INTD - 1))
		return -1;
	return irq - NUM_STD_IRQS + 1;
}

extern void _SysIntVecProgram(unsigned int vector, unsigned int);

#endif /* __INCboardh */
