/*
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief Board configuration macros for the ia32 platform
 *
 * This header file is used to specify and describe board-level aspects for
 * the 'ia32' platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

/*
 * UART
 */
#define UART_NS16550_ACCESS_IOPORT

#define UART_NS16550_PORT_0_BASE_ADDR		0x03F8
#define UART_NS16550_PORT_0_IRQ			4
#define UART_NS16550_PORT_0_CLK_FREQ		1843200

#define UART_NS16550_PORT_1_BASE_ADDR		0x02F8
#define UART_NS16550_PORT_1_IRQ			3
#define UART_NS16550_PORT_1_CLK_FREQ		1843200

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#define UART_IRQ_FLAGS				(IOAPIC_EDGE | IOAPIC_HIGH)
#endif /* CONFIG_IOAPIC */


#define INT_VEC_IRQ0 0x20 /* Vector number for IRQ0 */
/* PCI definitions */
#define PCI_BUS_NUMBERS 1

#define PCI_CTRL_ADDR_REG 0xCF8
#define PCI_CTRL_DATA_REG 0xCFC

#define PCI_INTA 1
#define PCI_INTB 2
#define PCI_INTC 3
#define PCI_INTD 4

/**
 *
 * @brief Convert PCI interrupt PIN to IRQ
 *
 * This file is only used by QEMU, which emulates the i440fx chipset.
 * INTx are mapped to IRQs 10 and 11 after being swizzled.
 *
 * @return IRQ number, -1 if the result is incorrect
 *
 */

static inline int pci_pin2irq(int bus, int dev, int pin)
{
	ARG_UNUSED(bus);

	if ((pin < PCI_INTA) || (pin > PCI_INTD))
		return -1;
	return 10 + (((pin + dev - 1) >> 1) & 1);
}

#endif /* __SOC_H_ */
