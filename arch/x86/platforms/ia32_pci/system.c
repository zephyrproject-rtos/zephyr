/* system.c - system/hardware module for the ia32_pci platform */

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
This module provides routines to initialize and support board-level hardware
for the ia32_pci platform.

Implementation Remarks:
Handlers for the secondary serial port have not been added.
 */

#include <nanokernel.h>
#include <init.h>
#include <device.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "board.h"
#include <drivers/uart.h>
#include <drivers/ioapic.h>
#include <drivers/pic.h>
#include <drivers/pci/pci.h>
#include <drivers/pci/pci_mgr.h>
#include <drivers/loapic.h>
#include <drivers/ioapic.h>

#ifdef CONFIG_HPET_TIMER
#include <drivers/hpet.h>
static inline void hpet_irq_set(void)
{
	_ioapic_irq_set(CONFIG_HPET_TIMER_IRQ,
			CONFIG_HPET_TIMER_IRQ + INT_VEC_IRQ0,
			HPET_IOAPIC_FLAGS);
}
#else
#define hpet_irq_set()   \
	do { /* nothing */       \
	} while ((0))
#endif

#ifdef CONFIG_CONSOLE_HANDLER
static inline void console_irq_set(void)
{
	_ioapic_irq_set(CONFIG_UART_CONSOLE_IRQ,
			CONFIG_UART_CONSOLE_IRQ + INT_VEC_IRQ0,
			UART_IOAPIC_FLAGS);
}
#else
#define console_irq_set()	\
	do { /* nothing */	\
	} while ((0))
#endif

#ifdef CONFIG_DW_I2C0
static inline void dw_i2c0_irq_set(void)
{
	_ioapic_irq_set(CONFIG_DW_I2C0_IRQ,
			CONFIG_DW_I2C0_IRQ + INT_VEC_IRQ0,
			DW_I2C0_IRQ_IOAPIC_FLAGS);
}
#else
#define dw_i2c0_irq_set()	\
	do { /* nothing */	\
	} while ((0))
#endif /* CONFIG_DW_I2C0 */

#ifdef CONFIG_GPIO_DW_0
static inline void gpio_irq_set(void) {
	_ioapic_irq_set(CONFIG_GPIO_DW_0_IRQ,
			CONFIG_GPIO_DW_0_IRQ + INT_VEC_IRQ0,
			GPIO_DW_0_IRQ_IOAPIC_FLAGS);
}
#else
#define gpio_irq_set()		\
	do { /* nothing */	\
	} while ((0))
#endif /* CONFIG_GPIO_DW_0 */

/**
 *
 * @brief Perform basic hardware initialization
 *
 * Initialize the Intel LOAPIC and IOAPIC device driver and the
 * Intel 8250 UART device driver.
 * Also initialize the timer device driver, if required.
 *
 * @return 0
 */

static int ia32_pci_init(struct device *arg)
{
	ARG_UNUSED(arg);

	hpet_irq_set();      /* NOP if not needed */
	console_irq_set();   /* NOP if not needed */
	gpio_irq_set();      /* NOP if not needed */

	dw_i2c0_irq_set();   /* NOP if not needed */

#if defined(CONFIG_PCI_DEBUG) && defined(CONFIG_PCI_ENUMERATION)
	/* Rescan PCI and display the list of PCI attached devices */
	struct pci_dev_info info = {
		.function = PCI_FUNCTION_ANY,
		.bar = PCI_BAR_ANY,
	};

	pci_bus_scan_init();

	while (pci_bus_scan(&info)) {
		pci_show(&info);
		info.class = 0;
		info.vendor_id = 0;
		info.device_id = 0;
		info.function = PCI_FUNCTION_ANY;
		info.bar = PCI_BAR_ANY;
	}
#endif /* CONFIG_PCI_DEBUG && CONFIG_PCI_ENUMERATION */
	return 0;
}

#ifdef CONFIG_IOAPIC
DECLARE_DEVICE_INIT_CONFIG(ioapic_0, "", _ioapic_init, NULL);
pure_early_init(ioapic_0, NULL);

#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_LOAPIC
DECLARE_DEVICE_INIT_CONFIG(loapic_0, "", _loapic_init, NULL);
pure_early_init(loapic_0, NULL);

#endif /* CONFIG_LOAPIC */

#if defined(CONFIG_PIC_DISABLE)

DECLARE_DEVICE_INIT_CONFIG(pic_0, "", _i8259_init, NULL);
pure_early_init(pic_0, NULL);

#endif /* CONFIG_PIC_DISABLE */

DECLARE_DEVICE_INIT_CONFIG(ia32_pci_0, "", ia32_pci_init, NULL);
pure_early_init(ia32_pci_0, NULL);
