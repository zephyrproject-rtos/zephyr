/* system.c - system/hardware module for the galileo platform */

/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
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
#include <uart.h>
#include <drivers/ioapic.h>
#include <drivers/pic.h>
#include <drivers/pci/pci.h>
#include <drivers/pci/pci_mgr.h>
#include <drivers/loapic.h>
#include <drivers/ioapic.h>
#include <drivers/hpet.h>


#ifdef CONFIG_I2C_DW_0
#ifdef CONFIG_I2C_DW_0_IRQ_DIRECT
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

#endif /* CONFIG_I2C_DW_0_IRQ_DIRECT */
#endif /* CONFIG_I2C_DW_0 */

#ifdef CONFIG_GPIO_DW_0
#ifdef CONFIG_GPIO_DW_0_IRQ_DIRECT
static int gpio_irq_set_0(struct device *unused)
{
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_GPIO_DW_0_IRQ,
			CONFIG_GPIO_DW_0_IRQ + INT_VEC_IRQ0,
			GPIO_DW_0_IRQ_IOAPIC_FLAGS);
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(gpioirq_0, "", gpio_irq_set_0, NULL);
pre_kernel_early_init(gpioirq_0, NULL);

#endif /* CONFIG_GPIO_DW_0_IRQ_DIRECT */
#endif /* CONFIG_GPIO_DW_0 */

#ifdef CONFIG_SPI_INTEL_PORT_0
static int spi_irq_set_0(struct device *unused)
{
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_SPI_INTEL_PORT_0_IRQ,
			CONFIG_SPI_INTEL_PORT_0_IRQ + INT_VEC_IRQ0,
			SPI_INTEL_IRQ_IOAPIC_FLAGS);
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(spiirq_0, "", spi_irq_set_0, NULL);
pre_kernel_early_init(spiirq_0, NULL);

#endif /* CONFIG_SPI_INTEL_PORT_0 */

#ifdef CONFIG_SPI_INTEL_PORT_1
static int spi_irq_set_1(struct device *unused)
{
	ARG_UNUSED(unused);
	_ioapic_irq_set(CONFIG_SPI_INTEL_PORT_1_IRQ,
			CONFIG_SPI_INTEL_PORT_1_IRQ + INT_VEC_IRQ0,
			SPI_INTEL_IRQ_IOAPIC_FLAGS);
	return 0;

}

DECLARE_DEVICE_INIT_CONFIG(spiirq_1, "", spi_irq_set_1, NULL);
pre_kernel_early_init(spiirq_1, NULL);

#endif /* CONFIG_SPI_INTEL_PORT_1 */

#ifdef CONFIG_SHARED_IRQ
#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>

#if defined(CONFIG_SHARED_IRQ_0_FALLING_EDGE)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_0_RISING_EDGE)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0_LEVEL_HIGH)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_0_LEVEL_LOW)
#define SHARED_IRQ_0_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#if defined(CONFIG_SHARED_IRQ_1_FALLING_EDGE)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_LOW)
#elif defined(CONFIG_SHARED_IRQ_1_RISING_EDGE)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_EDGE | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1_LEVEL_HIGH)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_HIGH)
#elif defined(CONFIG_SHARED_IRQ_1_LEVEL_LOW)
#define SHARED_IRQ_1_IOAPIC_FLAGS	(IOAPIC_LEVEL | IOAPIC_LOW)
#endif

#endif /* CONFIG_IOAPIC */
#endif /* CONFIG_SHARED_IRQ */

#ifdef CONFIG_PCI_LEGACY_BRIDGE
/**
 *
 * @brief Configure PCI interrupt pin to IRQ mapping
 *
 * The routine detects PCI legacy bridge and if present,
 * configures PCI interrupt pin to IRQ mapping for D:20
 * and D:21 IO Fabric, that contains the following devices:
 * - SPI0, SPI1;
 * - I2C;
 * - GPIO;
 * - UART0, UART1;
 * - SDIO/eMMC, USB, Ethernet.
 */
static int pci_legacy_bridge_irq_config(struct device *unused)
{
	ARG_UNUSED(unused);
	struct pci_dev_info info = {
		.function = PCI_FUNCTION_ANY,
		.bar = PCI_BAR_ANY,
	};
	if (pci_legacy_bridge_detect(&info) == 0) {
		pci_legacy_bridge_configure(&info, 1, PCI_INTA, 16);
		pci_legacy_bridge_configure(&info, 1, PCI_INTB, 17);
		pci_legacy_bridge_configure(&info, 1, PCI_INTC, 18);
		pci_legacy_bridge_configure(&info, 1, PCI_INTD, 19);
	}
	return 0;
}

DECLARE_DEVICE_INIT_CONFIG(pci_legacy_bridge_0, "", pci_legacy_bridge_irq_config, NULL);
pre_kernel_late_init(pci_legacy_bridge_0, NULL);
#endif /* CONFIG_PCI_LEGACY_BRIDGE */

#ifdef CONFIG_CONSOLE_HANDLER

static int console_irq_set(struct device *unsued)
{
	_ioapic_irq_set(CONFIG_UART_CONSOLE_IRQ,
			CONFIG_UART_CONSOLE_IRQ + INT_VEC_IRQ0,
			UART_IOAPIC_FLAGS);

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
pre_kernel_early_init(hpetirq, NULL);

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
pre_kernel_early_init(sharedirqcfg, NULL);

#endif /* CONFIG_SHARED_IRQ */
