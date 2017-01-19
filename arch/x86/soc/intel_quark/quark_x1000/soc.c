/*
 * Copyright (c) 2013-2015, Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file
 * @brief System/hardware module for the galileo platform
 *
 * This module provides routines to initialize and support board-level hardware
 * for the galileo platform.
 *
 * Implementation Remarks:
 * Handlers for the secondary serial port have not been added.
 */

#include <kernel.h>
#include <init.h>
#include <device.h>
#include <misc/printk.h>
#include <misc/__assert.h>
#include "soc.h"
#include <uart.h>
#include <drivers/pci/pci.h>
#include <drivers/pci/pci_mgr.h>
#include <drivers/ioapic.h>


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
		pci_legacy_bridge_configure(&info, 0, PCI_INTA, 17);
		pci_legacy_bridge_configure(&info, 0, PCI_INTB, 18);
		pci_legacy_bridge_configure(&info, 0, PCI_INTC, 19);
		pci_legacy_bridge_configure(&info, 0, PCI_INTD, 16);
	}
	return 0;
}

SYS_INIT(pci_legacy_bridge_irq_config,
		POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
#endif /* CONFIG_PCI_LEGACY_BRIDGE */
