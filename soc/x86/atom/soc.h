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

#include <sys/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <random/rand32.h>
#endif

#define INT_VEC_IRQ0 0x20 /* vector number for IRQ0 */

/*
 * UART
 */
#define UART_NS16550_ACCESS_IOPORT


/* PCI definitions */
/* FIXME: The values below copied from generic ia32 soc, we need to get the
 * correct numbers for Atom and the minnowboard
 *
 * This is added now to get basic enumartion of devices and verify that PCI
 * driver is functional.
 */
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
 * @return IRQ number, -1 if the result is incorrect
 *
 */

static inline int pci_pin2irq(int bus, int dev, int pin)
{
	ARG_UNUSED(bus);

	if ((pin < PCI_INTA) || (pin > PCI_INTD)) {
		return -1;
	}
	return 10 + (((pin + dev - 1) >> 1) & 1);
}

#endif /* __SOC_H_ */
