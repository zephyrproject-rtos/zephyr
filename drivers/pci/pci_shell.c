/*
 * Copyright (c) 2018 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <shell/shell.h>
#include <init.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <logging/log.h>
#include <pci/pci.h>

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

LOG_MODULE_REGISTER(lspci_shell);

static void list_devices(const struct shell *shell,
			 struct pci_dev_info *dev_info)
{

	shell_fprintf(shell, SHELL_NORMAL,
		      "%u:%u %X:%X class: 0x%X, %u, %u, %s, "
		      "addrs: 0x%X-0x%X, IRQ %d\n",
		      dev_info->bus,
		      dev_info->dev,
		      dev_info->vendor_id,
		      dev_info->device_id,
		      dev_info->class_type,
		      dev_info->function,
		      dev_info->bar,
		      (dev_info->mem_type == BAR_SPACE_MEM) ? "MEM" : "I/O",
		      (u32_t)dev_info->addr,
		      (u32_t)(dev_info->addr + dev_info->size - 1),
		      dev_info->irq);
}

static int cmd_lspci(const struct shell *shell, size_t argc, char **argv)
{
	struct pci_dev_info info = {
		.function = PCI_FUNCTION_ANY,
		.bar = PCI_BAR_ANY,
	};

	if (shell_help_requested(shell)) {
		shell_help_print(shell);
		return 1;
	}

	pci_bus_scan_init();

	while (pci_bus_scan(&info)) {
		list_devices(shell, &info);
		info.class_type = 0;
		info.vendor_id = 0;
		info.device_id = 0;
		info.function = PCI_FUNCTION_ANY;
		info.bar = PCI_BAR_ANY;
	}

	return 0;
}

SHELL_CMD_REGISTER(lspci, NULL, "List PCI devices", cmd_lspci);
