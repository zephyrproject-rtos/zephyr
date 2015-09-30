/* system.c - system/hardware module for the galileo platform */

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
for the galileo platform.

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
#include <drivers/hpet.h>


#ifdef CONFIG_I2C_DW_0
static int dw_i2c0_irq_set(struct device *unused)
{
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_I2C_DW_0_IRQ,
			CONFIG_I2C_DW_0_IRQ + INT_VEC_IRQ0,
			I2C_DW_IRQ_IOAPIC_FLAGS);
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(i2cirq_0, "", dw_i2c0_irq_set, NULL);
pre_kernel_late_init(i2cirq_0, NULL);

#endif /* CONFIG_I2C_DW_0 */

#ifdef CONFIG_GPIO_DW_0
static int gpio_irq_set_0(struct device *unused) {
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_GPIO_DW_0_IRQ,
			CONFIG_GPIO_DW_0_IRQ + INT_VEC_IRQ0,
			GPIO_DW_0_IRQ_IOAPIC_FLAGS);
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(gpioirq_0, "", gpio_irq_set_0, NULL);
pre_kernel_early_init(gpioirq_0, NULL);

#endif /* CONFIG_GPIO_DW_0 */

#ifdef CONFIG_SHARED_IRQ
#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>

#if defined(CONFIG_SHARED_IRQ_0_FALLING_EDGE)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_0__RISING_EDGE)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0__LEVEL_HIGH)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0__LEVEL_LOW)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#if defined(CONFIG_SHARED_IRQ_1_FALLING_EDGE)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_1__RISING_EDGE)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1__LEVEL_HIGH)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1__LEVEL_LOW)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_SHARED_IRQ */

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

static int galileo_init(struct device *arg)
{
	ARG_UNUSED(arg);

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

#ifdef CONFIG_CONSOLE_HANDLER

static int console_irq_set(struct device *unsued)
{
#if defined(CONFIG_UART_CONSOLE)
	_ioapic_irq_set(CONFIG_UART_CONSOLE_IRQ,
			CONFIG_UART_CONSOLE_IRQ + INT_VEC_IRQ0,
			UART_IOAPIC_FLAGS);
#endif
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(consoleirq, "", console_irq_set, NULL);
pre_kernel_late_init(consoleirq, NULL);

#endif /* CONFIG_CONSOLE_HANDLER */

#ifdef CONFIG_HPET_TIMER

static int hpet_irq_set(struct device *unused)
{
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_HPET_TIMER_IRQ,
			CONFIG_HPET_TIMER_IRQ + INT_VEC_IRQ0,
			HPET_IOAPIC_FLAGS);
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(hpetirq, "", hpet_irq_set, NULL);
pre_kernel_core_init(hpetirq, NULL);

#endif /* CONFIG_HPET_TIMER */

#ifdef CONFIG_IOAPIC
DECLARE_DEVICE_INIT_CONFIG(ioapic_0, "", _ioapic_init, NULL);
pre_kernel_core_init(ioapic_0, NULL);

#endif /* CONFIG_IOAPIC */

#ifdef CONFIG_LOAPIC
DECLARE_DEVICE_INIT_CONFIG(loapic_0, "", _loapic_init, NULL);
pre_kernel_core_init(loapic_0, NULL);

#endif /* CONFIG_LOAPIC */

#if defined(CONFIG_PIC_DISABLE)

DECLARE_DEVICE_INIT_CONFIG(pic_0, "", _i8259_init, NULL);
pre_kernel_core_init(pic_0, NULL);

#endif /* CONFIG_PIC_DISABLE */

#ifdef CONFIG_SHARED_IRQ

static int shared_irq_config(struct device *unused)
{
	ARG_UNUSED(unused);

#ifdef SHARED_IRQ_0_IOAPIC_FLAGS
	_ioapic_irq_set(CONFIG_SHARED_IRQ_0_IRQ,
			CONFIG_SHARED_IRQ_0_IRQ + INT_VEC_IRQ0,
			SHARED_IRQ_0_IOAPIC_FLAGS);
#endif

#ifdef SHARED_IRQ_1_IOAPIC_FLAGS
	_ioapic_irq_set(CONFIG_SHARED_IRQ_1_IRQ,
			CONFIG_SHARED_IRQ_1_IRQ + INT_VEC_IRQ0,
			SHARED_IRQ_1_IOAPIC_FLAGS);
#endif

	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(sharedirqcfg, "", shared_irq_config, NULL);
pre_kernel_late_init(sharedirqcfg, NULL);

#endif /* CONFIG_SHARED_IRQ */

DECLARE_DEVICE_INIT_CONFIG(galileo_0, "", galileo_init, NULL);
pre_kernel_early_init(galileo_0, NULL);
